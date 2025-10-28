#include <windows.h>
#include <windowsx.h>
#include <atomic>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "lua.hpp"

// NOTE(ivan): as far as I am concerned the lua_State
//             and whatever runs the lua code
//             is a single threaded thing.
//
//             Thus we can ignore mutex or atomics on things 
//             which are not used in the separate window, 
//             in our case it is only the Running variable
//
struct WinAPI_Runtime {
  std::atomic<bool> Running = { false };
  _beginthreadex_proc_type Task = NULL;
  HANDLE Handle = NULL;
  DWORD Id = 0;
  DWORD Timeout = INFINITE;
} g_WinAPI_Runtime = {};

int lua_StartThread(lua_State* L) {
  if (g_WinAPI_Runtime.Task == NULL) {
    return luaL_error(
      L, "[Plugin] : entry point is not set"
    );
  }
  if (g_WinAPI_Runtime.Running.load()) {
    return luaL_error(
      L, "[Plugin] : thread is already running"
    );
  }

  // NOTE(ivan): Standard WinAPI call would be something like CreateThread
  //             but apparentely you should use _beginthreadex.
  //             
  //             It sets up for us several things C Runtime Library needs.
  //             For example: thread local variables and structures.
  //             Because we are using Lua which uses C standard library.
  //             Or if we ever decide to use C/C++ stdlib anyway
  //             — we better use _beginthreadex 
  {
    unsigned int Id = 0;
    uintptr_t Handle = _beginthreadex(
        NULL, 
        0, 
        g_WinAPI_Runtime.Task, 
        NULL, 
        0, 
        &Id
    );
    g_WinAPI_Runtime.Handle = (HANDLE) Handle;
    g_WinAPI_Runtime.Id = Id;
  }

  if (g_WinAPI_Runtime.Handle == NULL) {
    return luaL_error(
      L, "[Plugin] : operating system failed to start a thread"
    );
  }

  g_WinAPI_Runtime.Running = true;

  return 0;
}

int lua_StopThread(lua_State* L) {
  g_WinAPI_Runtime.Running = false;
  DWORD Status;
  bool finished = false;

  // NOTE(ivan): We can't do anything really as far as I am concerned
  //             but to write code which is aware that it is running 
  //             in a separate thread and let it terminate.
  //
  //             If we want really something forcefull here
  //             we better use separate process entirely.
  //
  //             That actually might be a good idea.
  //             But then we have IPC business to resolve (shared memory pages for example) 
  //             or file/networking IO which is not the point here.
  //
  while (!finished) {
    Status = WaitForSingleObject(
        g_WinAPI_Runtime.Handle, 
        g_WinAPI_Runtime.Timeout
    );
    if (Status != WAIT_TIMEOUT) {
      finished = true;
    }
  }

  CloseHandle(g_WinAPI_Runtime.Handle);
  g_WinAPI_Runtime.Handle = NULL;
  g_WinAPI_Runtime.Id = 0;

  if (Status == WAIT_FAILED) {
    return luaL_error(
      L, "[Plugin] : failure during thread shutdown [%d]", 
      GetLastError()
    );
  }

  return 0;
}


int lua_OpenWindow(lua_State* L) {
  return 0;
}

int lua_CloseWindow(lua_State* L) {
  return 0;
}

int lua_GetRuntimeInfo(lua_State* L) {
  // Lua 5.2.3 does not handle 64 bit integers
  // that's why to display pointers we should just convert them to strings
  // 
  // 64 bit numbers are at max 20 digits long or 16 digits if they are printed hex
  // with 0x we kinda get 22, so let it be just 25 characters long string
  // 
  // booleans are handled correctly
  //
  constexpr uint16_t PointerDisplaySize = 25;
  
  bool      Running      = g_WinAPI_Runtime.Running.load();
  uintptr_t TaskFunction = (uintptr_t) g_WinAPI_Runtime.Task;
  uintptr_t ThreadHandle = (uintptr_t) g_WinAPI_Runtime.Handle;
  uint32_t  ThreadId     = (uint32_t)  g_WinAPI_Runtime.Id;

  char TaskString[PointerDisplaySize];
  char HandleString[PointerDisplaySize];

  snprintf(TaskString,   PointerDisplaySize, "0x%" PRIx64, TaskFunction);
  snprintf(HandleString, PointerDisplaySize, "0x%" PRIx64, ThreadHandle);
  
  lua_newtable(L);
  lua_pushboolean(L, Running);
  lua_setfield(L, -2, "running");
  lua_pushstring(L, TaskString);
  lua_setfield(L, -2, "task");
  lua_pushstring(L, HandleString);
  lua_setfield(L, -2, "threadHandle");
  lua_pushinteger(L, ThreadId);
  lua_setfield(L, -2, "threadId");

  return 1;
}

unsigned int ThreadTask(void*) {
  while (g_WinAPI_Runtime.Running.load()) {
    // do nothing
  }
  _endthreadex(0);
  return 0;
}

static const struct luaL_Reg WindowFunctions[] = {
  { "startThread", lua_StartThread },
  { "stopThread", lua_StopThread },
  { "open", lua_OpenWindow },
  { "close", lua_CloseWindow },
  { "getRuntimeInfo", lua_GetRuntimeInfo },
  { NULL, NULL }
};

extern "C" __declspec(dllexport) int luaopen_window(lua_State *L) {
  {
    g_WinAPI_Runtime.Task = ThreadTask;
  }
  luaL_newlib(L, WindowFunctions);
  return 1;
}
