#include <windows.h>
#include <windowsx.h>
#include "lua.hpp"
#include <stdio.h>

lua_State* g_L;
lua_State* cL;

HWND g_Window = NULL;
HWND g_ParentWindow = NULL;
HWND g_Trigger = NULL;
const char* g_EventDispatcherSignature = "QEventDispatcherWin32";
HWND g_EventDispatcher = NULL;

HWND Windows[1024] = {NULL};

char CellValue[2000] = "";

HANDLE g_hThread = NULL;
DWORD g_threadId = 0;

char g_ErrorMessage[4096] = "";

int DocumentX = 0;
int DocumentY = 0;

int g_InvokedTimes = 0;

int g_EditorAPI = LUA_NOREF;
int g_Increase = LUA_NOREF;
int g_Select = LUA_NOREF;
int g_Context = LUA_NOREF;

const int MAX_KEYS_PRESSED = 7;
char KeysPressed[] = {0, 0, 0, 0, 0, 0, 0};
int CurrentKey = 0;
int PressedTotal = 0;

BOOL CALLBACK etwc(HWND Window, LPARAM param) {
  HWND** result = (HWND**) param;
  char buffer[2028];
  GetClassNameA(Window, buffer, sizeof(buffer));

  **result = Window;
  *result += 1;

  return TRUE;
}

POINT Mouse = {0, 0};

void RenderScene(HDC Context, HWND Window) {
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);

  HBRUSH WhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
  FillRect(Context, &ClientRect, WhiteBrush);
  DeleteObject(WhiteBrush);

  HPEN RedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  HPEN OldPen = (HPEN) SelectObject(Context, RedPen);

  Rectangle(Context, 50, 50, 350, 150);
  
  SelectObject(Context, OldPen);
  DeleteObject(RedPen);

  SetTextColor(Context, RGB(0, 0, 255));
  SetBkMode(Context, TRANSPARENT);
  
  const char* hello = "Hello world! Version 2";
  const char* from = "USING WIN32API GDI";
  TextOutA(Context, 60, 60, hello, strlen(hello));
  TextOutA(Context, 60, 80, from, strlen(from));
  
  const char* pressed = "Last pressed: ";
  char one[2] = {'1', '\0'};
  const char* comma = ",";

  TextOutA(Context, 60, 100, pressed, strlen(pressed));
  int UseKey = CurrentKey;
  for (int i = 0; 
       ((i < MAX_KEYS_PRESSED)          
       && (i < PressedTotal));
       i += 1) {
    UseKey = (UseKey - 1 + MAX_KEYS_PRESSED) % MAX_KEYS_PRESSED;
    one[0] = KeysPressed[UseKey];
    TextOutA(Context, 120 + 60 + i * 20, 100, one, 1);
    TextOutA(Context, 120 + 60 + i * 20 + 16, 100, comma, 1);
  }

  char buffer[5000];
  sprintf_s(buffer, "Mouse: %d %d", Mouse.x, Mouse.y);
  TextOutA(Context, 60, 180, buffer, strlen(buffer));

  sprintf_s(buffer, "Trigger: %p", g_Trigger);
  TextOutA(Context, 60, 200, buffer, strlen(buffer));

  sprintf_s(buffer, "ParentWindow: %p %p", g_ParentWindow, GetWindow(g_Window, GW_OWNER));
  TextOutA(Context, 60, 220, buffer, strlen(buffer));

  sprintf_s(buffer, "EventDispatcher: %p", g_EventDispatcher);
  TextOutA(Context, 60, 240, buffer, strlen(buffer));

  void* ThreadId = (void*) (long long) GetCurrentThreadId();
  sprintf_s(buffer, "Thread: %p", ThreadId);
  TextOutA(Context, 60, 260, buffer, strlen(buffer));
  
  const char * lua_value;
  if (g_L) {
        int top = lua_gettop(g_L);
        lua_getglobal(g_L, "MY_GLOBAL_TEST_STRING");
        
        if (lua_isstring(g_L, -1)) {
            lua_value = lua_tostring(g_L, -1);
        } else {
            lua_value = "MY_GLOBAL_TEST_STRING is not a string (or is nil)";
        }
        lua_settop(g_L, top);
    } else {
        lua_value = "gL is NULL";
    }

    // Рисуем полученное значение в нашем окне
    const char* prefix = "Global from Lua: ";
    TextOutA(Context, 60, 280, prefix, strlen(prefix));
    TextOutA(Context, 60 + 10 * strlen(prefix), 280, lua_value, strlen(lua_value));

    // sprintf_s(buffer, "g_BoxFunction: %p", g_MessageBoxFunction);
    // TextOutA(Context, 60, 280, buffer, strlen(buffer));

    sprintf_s(buffer, "InvokedTimes [%d] ErrorMessage: `%s` Value: [%s]", g_InvokedTimes, g_ErrorMessage, CellValue);
    TextOutA(Context, 60, 320, buffer, strlen(buffer));
    
    HWND* p = Windows;
    HWND** t = &p;
    EnumThreadWindows(GetCurrentThreadId(), etwc, (LPARAM) t);
    **t = NULL;

    for (int i = 0; Windows[i] != NULL; ++i) {
      sprintf_s(buffer, "Found Window: [%d] %p", i, Windows[i]);
      TextOutA(Context, 60, 340 + i * 20, buffer, strlen(buffer));
    }
}



void InvokeMessageBox(const char* name="default") {
  if (g_L && (g_EditorAPI != LUA_NOREF)) {
    int top = lua_gettop(g_L);

    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_EditorAPI);
    lua_getfield(g_L, -1, "messageBox");

    char buffer[400];
    sprintf_s(buffer, "this EditorAPI.messageBox is invoked by C++: %d", g_InvokedTimes);
    lua_pushstring(g_L, buffer);
    lua_pushstring(g_L, name);

    if (lua_pcall(g_L, 2, 0, 0) != LUA_OK) {
      strcpy(g_ErrorMessage, lua_tostring(g_L, -1));
    }
    g_InvokedTimes += 1;

    lua_settop(g_L, top);
  }
}

void SelectCell() {
  if (g_L && (g_Select != LUA_NOREF) 
          && (g_EditorAPI != LUA_NOREF) 
          && (g_Context != LUA_NOREF)) {
    int top = lua_gettop(g_L);
    
    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_Select);

    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_EditorAPI);
    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_Context);
    lua_pushinteger(g_L, DocumentX);
    lua_pushinteger(g_L, DocumentY);

    if (lua_pcall(g_L, 4, 1, 0) != LUA_OK) {
      strcpy(g_ErrorMessage, lua_tostring(g_L, -1));
    }
    // strcpy(CellValue, lua_tostring(g_L, -1));

    g_InvokedTimes += 1;

    lua_settop(g_L, top);
  } else {
    sprintf_s(g_ErrorMessage, 
        "g_Increase: [%d], g_EditorAPI [%d], g_Document [%d]", 
        g_Select, g_EditorAPI, g_Context);
  }
  SetFocus(g_ParentWindow);
  SetFocus(g_Window);
}

void IncreaseValue() {
  if (g_L && (g_Increase != LUA_NOREF) 
          && (g_EditorAPI != LUA_NOREF) 
          && (g_Context != LUA_NOREF)) {
    int top = lua_gettop(g_L);
    
    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_Increase);

    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_EditorAPI);
    lua_rawgeti(g_L, LUA_REGISTRYINDEX, g_Context);
    lua_pushinteger(g_L, DocumentX);
    lua_pushinteger(g_L, DocumentY);

    if (lua_pcall(g_L, 4, 1, 0) != LUA_OK) {
      strcpy(g_ErrorMessage, lua_tostring(g_L, -1));
    }
    // strcpy(CellValue, lua_tostring(g_L, -1));

    g_InvokedTimes += 1;

    lua_settop(g_L, top);
  } else {
    sprintf_s(g_ErrorMessage, 
        "g_Increase: [%d], g_EditorAPI [%d], g_Document [%d]", 
        g_Increase, g_EditorAPI, g_Context);
  }
  SetFocus(g_ParentWindow);
  SetFocus(g_Window);
}


LRESULT CALLBACK WndProc(HWND Window, UINT msg, WPARAM WParam, LPARAM LParam) {
  switch (msg) {
    case WM_MOUSEMOVE: {
      Mouse.x = GET_X_LPARAM(LParam);
      Mouse.y = GET_Y_LPARAM(LParam);

      InvalidateRect(Window, NULL, TRUE);
      break;
    }
    case WM_KEYDOWN: {
      char Pressed = (char) WParam;
      if (Pressed == ' ') {
        IncreaseValue();
      }
      if (isalnum(Pressed)) {
        KeysPressed[CurrentKey] = Pressed;

        CurrentKey += 1;
        PressedTotal += 1;
        CurrentKey %= MAX_KEYS_PRESSED;
        
        if (Pressed == 'W') {
          if (DocumentY > 0) {
            DocumentY -= 1;
            SelectCell();
          }
        } else if (Pressed == 'A') {
          if (DocumentX > 0) {
            DocumentX -= 1;
            SelectCell();
          }
        } else if (Pressed == 'D') {
          DocumentX += 1;
          SelectCell();
        } else if (Pressed == 'S') {
          DocumentY += 1;
          SelectCell();
        }
      }
      InvalidateRect(Window, NULL, TRUE);
      break;
    }
    case WM_CLOSE: {
      DestroyWindow(Window);
      break;
    }
    case WM_DESTROY: {
      g_Window = NULL;
      // PostQuitMessage(0);
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT Paint;
      HDC Context = BeginPaint(Window, &Paint);

      RECT ClientRect; 
      GetClientRect(Window, &ClientRect);

      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;

      HDC BufferedCtx = CreateCompatibleDC(Context);
      HBITMAP Bitmap = CreateCompatibleBitmap(Context, Width, Height);
      HGDIOBJ Initial = SelectObject(BufferedCtx, Bitmap);

      RenderScene(BufferedCtx, Window);
      BitBlt(Context, 0, 0, Width, Height, BufferedCtx, 0, 0, SRCCOPY);

      SelectObject(BufferedCtx, Initial);

      DeleteObject(Bitmap);
      DeleteDC(BufferedCtx);

      EndPaint(Window, &Paint);
      break;
    }
    case WM_LBUTTONDOWN: {
      InvalidateRect(Window, NULL, TRUE);
      break;
    }
    default: {
      return DefWindowProcA(Window, msg, WParam, LParam);
    }
  }
  return 0;
}


////////////////////////////
// Lua interfaces 
// TODO(ivan)
//


static int lua_OpenWindow(lua_State *L) {
  g_ParentWindow = FindWindowA("Qt51510QWindowIcon", NULL);
  HWND* p = &(Windows[0]);
  HWND** t = & p;
  EnumThreadWindows(GetCurrentThreadId(), etwc, (LPARAM) t);

  if (g_Window != NULL) {
    cL = L;
    //lua_pushstring(L, "Window is already open");
    //return lua_error(L);
    return 0;
  }
  g_L = L;
  
  lua_pushvalue(L, 1);
  g_Context = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_getglobal(L, "EditorAPI");
  if (lua_istable(L, -1)) {
    g_EditorAPI = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
  }

  lua_getglobal(L, "updateNumber");
  if (lua_isfunction(L, -1)) {
    g_Increase = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
  }

  lua_getglobal(L, "select");
  if (lua_isfunction(L, -1)) {
    g_Select = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    lua_pop(L, 1);
  }
  
  static const char* ClassName = "test-plugin"; 
  HINSTANCE HInstance = GetModuleHandleA(NULL);

  WNDCLASSEXA Wc = {0};
  Wc.cbSize = sizeof(WNDCLASSEXA);
  Wc.lpfnWndProc = WndProc;
  Wc.hInstance = HInstance;
  Wc.lpszClassName = ClassName;
  Wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  RegisterClassExA(&Wc);

  HWND HandleToWindow = CreateWindowExA(
      WS_EX_TOPMOST,             // NOTE(ivan): - create window on top
      ClassName,                 //             - pass some class name
      "test-plugin",             //             - window title
      WS_OVERLAPPEDWINDOW,       //             - default style
      CW_USEDEFAULT,             //             - use some default position TODO(ivan): should be something elese
      CW_USEDEFAULT,             //             - use some default position TODO(ivan): should be something elese
      400,                       //             - width TODO(ivan): should be something else
      300,                       //             - height TODO(ivan): should be something else
      g_ParentWindow,            //             - parent_window 
      NULL,                      //             - menu (no menu right now)
      HInstance,                 //             - app instance TODO(ivan): maybe change it
      NULL                       //             - some user data
  );



  if (HandleToWindow == NULL) {
    return 1;
  }
  // SetParent(HandleToWindow, g_ParentWindow);

  g_Window = HandleToWindow;

  if (g_Window== NULL) {
    lua_pushstring(L, "Failed to create window");
    lua_error(L);
  }

  ShowWindow(g_Window, SW_SHOW);
  UpdateWindow(g_Window);

  // InvokeMessageBox();

  return 0;
}

static int lua_ReplaceContext(lua_State *L) {
  g_Context = LUA_NOREF;
  lua_pushvalue(L, 1);
  g_Context = luaL_ref(L, LUA_REGISTRYINDEX);
  return 0;
}

static int lua_CloseWindow(lua_State *L) {
  if (g_Window == NULL) {
    return 0;
  }

  PostMessageA(g_Window, WM_CLOSE, 0, 0);
  // WaitForSingleObject(g_hThread, 2000);
  g_Window = NULL;

  return 0;
} 

static const struct luaL_Reg window_functions[] = {
  { "open", lua_OpenWindow },
  { "close", lua_CloseWindow },
  { "replaceContext", lua_ReplaceContext },
  { NULL, NULL }
};

extern "C" __declspec(dllexport) int luaopen_window(lua_State *L) {
  luaL_newlib(L, window_functions);
  return 1;
}
