#include <windows.h>
#include <windowsx.h>
#include "lua.hpp"
#include <stdio.h>;

HWND g_hwnd = NULL;
HANDLE g_hThread = NULL;
DWORD g_threadId = 0;

const int MAX_KEYS_PRESSED = 7;
char KeysPressed[] = {0, 0, 0, 0, 0, 0, 0};
int CurrentKey = 0;
int PressedTotal = 0;

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

  char buffer[128];
  sprintf_s(buffer, "Mouse: %d %d", Mouse.x, Mouse.y);
  TextOutA(Context, 60, 180, buffer, strlen(buffer));
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
      if (isalnum(Pressed)) {
        KeysPressed[CurrentKey] = Pressed;

        CurrentKey += 1;
        PressedTotal += 1;
        CurrentKey %= MAX_KEYS_PRESSED;
      }

      InvalidateRect(Window, NULL, TRUE);
      break;
    }
    case WM_CLOSE: {
      DestroyWindow(Window);
      break;
    }
    case WM_DESTROY: {
      g_hwnd = NULL;
      PostQuitMessage(0);
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
    default: {
      return DefWindowProcA(Window, msg, WParam, LParam);
    }
  }
  return 0;
}


DWORD WINAPI WindowThread(LPVOID LParam) {
  static const char* ClassName = "template-plugin-class"; 
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
      NULL,                      //             - parent_window TODO(ivan): currently none, change it
      NULL,                      //             - menu (no menu right now)
      HInstance,                 //             - app instance TODO(ivan): maybe change it
      NULL                       //             - some user data
  );


  if (HandleToWindow == NULL) {
    return 1;
  }

  g_hwnd = HandleToWindow;

  ShowWindow(g_hwnd, SW_SHOW);
  UpdateWindow(g_hwnd);

  MSG msg;
  while (GetMessageA(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
 
  // some clean up when we finish
  g_hwnd = NULL;
  CloseHandle(g_hThread);
  g_hThread = NULL;
  g_threadId = 0;

  return 0;
}

////////////////////////////
// Lua interfaces 
// TODO(ivan)
//

static int lua_OpenWindow(lua_State *L) {
  if (g_hThread != NULL) {
    lua_pushstring(L, "Window is already open");
    return lua_error(L);
  }

  g_hThread = CreateThread(
      NULL, 0, WindowThread, NULL, 0, &g_threadId
  );

  if (g_hThread == NULL) {
    lua_pushstring(L, "Failed to create thread");
    lua_error(L);
  }

  return 0;
}

static int lua_CloseWindow(lua_State *L) {
  if (g_hThread == NULL || g_hwnd == NULL) {
    return 0;
  }

  PostMessageA(g_hwnd, WM_CLOSE, 0, 0);
  WaitForSingleObject(g_hThread, 2000);

  return 0;
} 

static const struct luaL_Reg window_functions[] = {
  { "open", lua_OpenWindow },
  { "close", lua_CloseWindow },
  { NULL, NULL }
};

extern "C" __declspec(dllexport) int luaopen_window(lua_State *L) {
  luaL_newlib(L, window_functions);
  return 1;
}
