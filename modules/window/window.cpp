#include <windows.h>
#include <windowsx.h>
#include <atomic>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "lua.hpp"

// 
// (#process-window-events)
//

constexpr UINT WM_PLUGIN_MESSAGE = WM_APP + 2000;
constexpr UINT WM_PLUGIN_CREATE_WINDOW = WM_PLUGIN_MESSAGE + 1;
constexpr UINT WM_PLUGIN_CLOSE_WINDOW  = WM_PLUGIN_MESSAGE + 2;
constexpr UINT WM_PLUGIN_STOP_THREAD   = WM_PLUGIN_MESSAGE + 3;

// NOTE(ivan): as far as I am concerned the lua_State
//             and whatever runs the lua code
//             is a single threaded thing.
//
//             Thus we can ignore mutex or atomics on things 
//             which are not used in the separate window, 
//             in our case it is only the Running and Window variables
//
struct WinAPI_Runtime {
  std::atomic<HWND> Window = { NULL };
  std::atomic<bool> Running = { false };
  _beginthreadex_proc_type Task = NULL;
  HANDLE IndieHandle = NULL;
  DWORD IndieId = 0;
  HANDLE MainHandle = NULL;
  DWORD MainId = 0;
  DWORD Timeout = INFINITE;
} g_WinAPI_Runtime = {};

LRESULT ProcessWindowEvents(HWND Window, UINT msg, WPARAM WParam, LPARAM LParam);



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
    g_WinAPI_Runtime.IndieHandle = (HANDLE) Handle;
    g_WinAPI_Runtime.IndieId = Id;
  }

  if (g_WinAPI_Runtime.IndieHandle == NULL) {
    return luaL_error(
      L, "[Plugin] : operating system failed to start a thread"
    );
  }

  g_WinAPI_Runtime.Running = true;

  return 0;
}

int lua_StopThread(lua_State* L) {
  bool Running = g_WinAPI_Runtime.Running.load();
  bool NotRunning = !Running;
  if (NotRunning) {
    return luaL_error(
      L, "[Plugin] : no thread is running"
    );
  }

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
  //
  PostThreadMessageW(
      g_WinAPI_Runtime.IndieId, 
      WM_PLUGIN_STOP_THREAD, 
      0, NULL
  );

  bool finished = false;
  DWORD Status;
  while (!finished) {
    Status = WaitForSingleObject(
        g_WinAPI_Runtime.IndieHandle, 
        g_WinAPI_Runtime.Timeout
    );
    if (Status != WAIT_TIMEOUT) {
      finished = true;
    }
  }

  CloseHandle(g_WinAPI_Runtime.IndieHandle);
  g_WinAPI_Runtime.IndieHandle = NULL;
  g_WinAPI_Runtime.IndieId = 0;

  if (Status == WAIT_FAILED) {
    return luaL_error(
      L, "[Plugin] : failure during thread shutdown [%d]", 
      GetLastError()
    );
  }

  return 0;
}


int lua_OpenWindow(lua_State* L) {
  if (!g_WinAPI_Runtime.Running.load()) {
    return luaL_error(L, "[Plugin] : thread is not running");
  }
  if (g_WinAPI_Runtime.Window.load() != NULL) {
    return luaL_error(L, "[Plugin] : window is already open");
  }

  DWORD ThreadId = g_WinAPI_Runtime.IndieId;
  UINT CreateWindowMessage = WM_PLUGIN_CREATE_WINDOW;
  WPARAM UnusedTag = 0;
  LPARAM UnusedPayload = NULL;

  PostThreadMessageW(
    ThreadId,
    CreateWindowMessage,
    UnusedTag,
    UnusedPayload
  );

  return 0;
}

int lua_CloseWindow(lua_State* L) {
  if (!g_WinAPI_Runtime.Running.load()) {
    return luaL_error(L, "[Plugin] : thread is not running");
  }
  if (g_WinAPI_Runtime.Window.load() == NULL) {
    return luaL_error(L, "[Plugin] : no window to close");
  }

  DWORD ThreadId = g_WinAPI_Runtime.IndieId;
  UINT CloseWindowMessage = WM_PLUGIN_CLOSE_WINDOW;
  WPARAM UnusedTag = 0;
  LPARAM UnusedPayload = NULL;

  BOOL Status = PostThreadMessageW(
    ThreadId,
    CloseWindowMessage,
    UnusedTag,
    UnusedPayload
  );

  if (! Status) {
    return luaL_error(
      L, "[Plugin] : failed to send WM_PLUGIN_CLOSE_WINDOW message"
    );
  }
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
  // TODO(ivan): maybe do the pointers and handles somewhat nicer to see
  //             but mostly they are for debug purposes not really to do 
  //             anything with them
  //
  constexpr uint16_t PointerDisplaySize = 25;
  
  uintptr_t Window      = (uintptr_t) g_WinAPI_Runtime.Window.load();
  bool      Running     = g_WinAPI_Runtime.Running.load();
  uintptr_t Task        = (uintptr_t) g_WinAPI_Runtime.Task;
  uintptr_t IndieHandle = (uintptr_t) g_WinAPI_Runtime.IndieHandle;
  uint32_t  IndieId     = (uint32_t)  g_WinAPI_Runtime.IndieId;
  uintptr_t MainHandle  = (uintptr_t) g_WinAPI_Runtime.MainHandle;
  uint32_t  MainId      = (uint32_t)  g_WinAPI_Runtime.MainId;

  char WindowString[PointerDisplaySize];
  char TaskString[PointerDisplaySize];
  char IndieHandleString[PointerDisplaySize];
  char MainHandleString[PointerDisplaySize];

  snprintf(WindowString,      PointerDisplaySize, "0x%" PRIx64, Window);
  snprintf(TaskString,        PointerDisplaySize, "0x%" PRIx64, Task);
  snprintf(IndieHandleString, PointerDisplaySize, "0x%" PRIx64, IndieHandle);
  snprintf(MainHandleString,  PointerDisplaySize, "0x%" PRIx64, MainHandle);
  
  lua_newtable(L);
  lua_pushstring(L, WindowString);
  lua_setfield(L, -2, "window");
  lua_pushboolean(L, Running);
  lua_setfield(L, -2, "running");
  lua_pushstring(L, TaskString);
  lua_setfield(L, -2, "task");
  lua_pushstring(L, IndieHandleString);
  lua_setfield(L, -2, "indieHandle");
  lua_pushinteger(L, IndieId);
  lua_setfield(L, -2, "indieId");
  lua_pushstring(L, MainHandleString);
  lua_setfield(L, -2, "mainHandle");
  lua_pushinteger(L, MainId);
  lua_setfield(L, -2, "mainId");

  return 1;
}

// (#process-window-events)

LRESULT ProcessWindowEvents(HWND Window, UINT msg, WPARAM WParam, LPARAM LParam) {
  HWND CurrentThread = NULL;

  switch (msg) {
    case WM_CLOSE: {
      PostMessageW(CurrentThread, WM_PLUGIN_CLOSE_WINDOW, 0, NULL);
      break;
    }
    default: {
      return DefWindowProcW(Window, msg, WParam, LParam);
    }
  }
  return 0;
}

HWND MakePluginWindow() {
  const wchar_t* ClassName = L"MyOfficeExtensionClass"; 
  HINSTANCE HInstance = GetModuleHandleW(NULL);

  WNDCLASSEXW Wc = {0};
  Wc.cbSize = sizeof(WNDCLASSEXW);
  Wc.lpfnWndProc = ProcessWindowEvents;
  Wc.hInstance = HInstance;
  Wc.lpszClassName = ClassName;
  Wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  RegisterClassExW(&Wc);

  HWND HandleToWindow = CreateWindowExW(
      WS_EX_TOPMOST,             // NOTE(ivan): - create window on top
      ClassName,                 //             - pass some class name
      L"Extension",              //             - window title
      WS_OVERLAPPEDWINDOW,       //             - default style
      CW_USEDEFAULT,             //             - use some default position TODO(ivan): should be something elese
      CW_USEDEFAULT,             //             - use some default position TODO(ivan): should be something elese
      400,                       //             - width TODO(ivan): should be something else
      300,                       //             - height TODO(ivan): should be something else
      NULL,                      //             - parent_window TODO(ivan): currently none, change it
      NULL,                      //             - menu (no menu right now)
      HInstance,                 //             - app instance TODO(ivan): maybe change it
      NULL                       //             - some user data
  );

  return HandleToWindow;
}

unsigned int ThreadTask(void*) {
  // broadcast that we are running

  HWND Window = NULL;
  HWND ThreadHandle = NULL;
  MSG  ThreadMessage = {};
  UINT FilterMinSetting = 0;  
  UINT FilterMaxSetting = 0;
  UINT RemoveMsgSetting = PM_REMOVE;
  
  bool Running = true;

  // broadcast the state
  auto BroadcastState = [&](void) -> void
  {
    g_WinAPI_Runtime.Window = Window;
    g_WinAPI_Runtime.Running = Running;
  };

  while (Running) {
    // process events 
    while (      
      PeekMessageW(
        &ThreadMessage,  
        ThreadHandle,
        FilterMinSetting,
        FilterMaxSetting,
        RemoveMsgSetting
    )) 
    {
      switch (ThreadMessage.message) {

        case WM_PLUGIN_STOP_THREAD: {
          Running = false;
          break;
        }

        case WM_PLUGIN_CREATE_WINDOW: {
          Window = MakePluginWindow();
          if (Window) {
            ShowWindow(Window, SW_SHOW);
            UpdateWindow(Window);
          } else {
            // TODO(ivan): failed to create window? log it?
          }

          BroadcastState();
          break;
        }

        case WM_PLUGIN_CLOSE_WINDOW: {
          if (DestroyWindow(Window)) {
            Window = NULL;
          } else {
            // TODO(ivan): failed to close window?? log it??
          }

          BroadcastState();
          break;
        }

        default: {
          DispatchMessageW(&ThreadMessage);
          break;
        }
      }
    }

    // do something else
  }
  
  // resources cleanup section 
  if (Window != NULL) {
    DestroyWindow(Window);
    Window = NULL;
  }
  BroadcastState();  

  _endthreadex(0);
  return 0;
}

static const struct luaL_Reg WindowFunctions[] = {
  { "startThread", lua_StartThread },
  { "stopThread", lua_StopThread },
  { "openWindow", lua_OpenWindow },
  { "closeWindow", lua_CloseWindow },
  { "getRuntimeInfo", lua_GetRuntimeInfo },
  { NULL, NULL }
};

extern "C" __declspec(dllexport) int luaopen_window(lua_State *L) {
  {
    g_WinAPI_Runtime.Task = ThreadTask;
    g_WinAPI_Runtime.MainId = GetCurrentThreadId();
    g_WinAPI_Runtime.MainHandle = OpenThread(SYNCHRONIZE, FALSE, g_WinAPI_Runtime.MainId);
  }
  luaL_newlib(L, WindowFunctions);
  return 1;
}
