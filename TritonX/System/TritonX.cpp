#include "pch.h"

#include "Graphics/Context.h"
#include "Logger.h"

#pragma warning(disable : 4061)

using namespace Microsoft::WRL;

namespace {
   std::unique_ptr<TX::Graphics::Context> context;
}

const LPCWSTR appName = L"TritonX";
const LPCWSTR className = L"TritonXClassName";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ const HINSTANCE hPrevInstance,
                    _In_ const PWSTR lpCmdLine,
                    _In_ int nCmdShow) {
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   Log::LogManager::getInstance().setMinLevel(Log::Level::Debug);

   Log::info << L"Logger Configured" << std::endl;

   if (!DirectX::XMVerifyCPUSupport())
      return 1;

   Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
   if (FAILED(initialize)) {
      Log::LastError();
      return 1;
   }

   context = std::make_unique<TX::Graphics::Context>();

   {
      // Register class
      WNDCLASSEXW wcex = {};
      wcex.cbSize = sizeof(WNDCLASSEXW);
      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = WndProc;
      wcex.hInstance = hInstance;
      wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
      wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
      wcex.lpszClassName = className;
      if (!RegisterClassExW(&wcex)) {
         Log::LastError();
         return 1;
      }

      // Create window
      int w = 0;
      int h = 0;
      context->GetDefaultSize(w, h);

      RECT rc = {0, 0, static_cast<LONG>(w), static_cast<LONG>(h)};

      AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

      HWND hwnd = CreateWindowExW(0,
                                  className,
                                  appName,
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  rc.right - rc.left,
                                  rc.bottom - rc.top,
                                  nullptr,
                                  nullptr,
                                  hInstance,
                                  context.get());
      // TODO: Change to CreateWindowExW(WS_EX_TOPMOST, L"D3D12Win32WindowClass", g_szAppName,
      // WS_POPUP, to default to fullscreen.

      if (!hwnd) {
         Log::LastError();
         return 1;
      }

      ShowWindow(hwnd, nCmdShow);
      // TODO: Change nCmdShow to SW_SHOWMAXIMIZED to default to fullscreen.

      GetClientRect(hwnd, &rc);

      context->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);
   }

   // Main message loop
   MSG msg = {};
   while (WM_QUIT != msg.message) {
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      } else {
         context->Tick();
      }
   }

   context.reset();

   return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   static bool inSizeMove = false;
   static bool inSuspend = false;
   static bool minimized = false;
   static bool fullscreen = false;
   // TODO: Set fullscreen to true if defaulting to fullscreen.

   auto context = reinterpret_cast<TX::Graphics::Context*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

   switch (message) {
      case WM_CREATE:
         if (lParam) {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(
                hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
         }
         break;

      case WM_PAINT:
         if (inSizeMove && context) {
            context->Tick();
         } else {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
         }
         break;

      case WM_SIZE:
         if (wParam == SIZE_MINIMIZED) {
            if (!minimized) {
               minimized = true;
               if (!inSuspend && context)
                  context->OnSuspending();
               inSuspend = true;
            }
         } else if (minimized) {
            minimized = false;
            if (inSuspend && context)
               context->OnResuming();
            inSuspend = false;
         } else if (!inSizeMove && context) {
            context->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
         }
         break;

      case WM_ENTERSIZEMOVE:
         inSizeMove = true;
         break;

      case WM_EXITSIZEMOVE:
         inSizeMove = false;
         if (context) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            context->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
         }
         break;

      case WM_GETMINMAXINFO:
         if (lParam) {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 426;
            info->ptMinTrackSize.y = 240;
         }
         break;

      case WM_ACTIVATEAPP:
         if (context) {
            if (wParam) {
               context->OnActivated();
            } else {
               context->OnDeactivated();
            }
         }
         break;

      case WM_POWERBROADCAST:
         switch (wParam) {
            case PBT_APMQUERYSUSPEND:
               if (!inSuspend && context)
                  context->OnSuspending();
               inSuspend = true;
               return TRUE;

            case PBT_APMRESUMESUSPEND:
               if (!minimized) {
                  if (inSuspend && context)
                     context->OnResuming();
                  inSuspend = false;
               }
               return TRUE;
         }
         break;

      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      case WM_SYSKEYDOWN:
         if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000) {
            // Implements the classic ALT+ENTER fullscreen toggle
            if (fullscreen) {
               SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
               SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

               int width = 800;
               int height = 600;
               if (context) {
                  context->GetDefaultSize(width, height);
               }

               ShowWindow(hWnd, SW_SHOWNORMAL);

               SetWindowPos(hWnd,
                            HWND_TOP,
                            0,
                            0,
                            width,
                            height,
                            SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            } else {
               SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
               SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

               SetWindowPos(hWnd,
                            HWND_TOP,
                            0,
                            0,
                            0,
                            0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

               ShowWindow(hWnd, SW_SHOWMAXIMIZED);
            }

            fullscreen = !fullscreen;
         }
         break;

      case WM_MENUCHAR:
         return MAKELRESULT(0, MNC_CLOSE);
   }

   return DefWindowProc(hWnd, message, wParam, lParam);
}

// Exit helper
static void ExitGame() noexcept {
   PostQuitMessage(0);
}