#include "pch.h"

#include "MainWindow.h"
namespace TX::System {
   MainWindow::MainWindow(HINSTANCE hInstance, int nShowCmd) noexcept :
       hInstance(hInstance), hWnd(nullptr), showCmd(nShowCmd), resizeHandlers({}) {
   }

   LRESULT CALLBACK MainWindow::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
      MainWindow* pMainWindow =
          InstanceFromWndProc<MainWindow, MainWindow, &MainWindow::hWnd>(hWnd, uMsg, lParam);

      if (pMainWindow) {
         switch (uMsg) {
            case WM_CREATE:
               return pMainWindow->OnCreate();
            case WM_DESTROY:
               return pMainWindow->OnDestroy();
            case WM_SIZE:
               return pMainWindow->OnSize();
         }
      }

      return DefWindowProcW(hWnd, uMsg, wParam, lParam);
   }

   LRESULT
   MainWindow::OnCreate() {
      SetWindowPos(hWnd, nullptr, 0, 0, 800, 600, SWP_NOMOVE);
      // Finally, show the window.
      ShowWindow(hWnd, showCmd);
      return 0;
   }

   LRESULT
   MainWindow::OnDestroy() {
      PostQuitMessage(0);
      return 0;
   }

   LRESULT
   MainWindow::OnSize() {
      RECT rcWindow;
      GetClientRect(hWnd, &rcWindow);
      return 0;
   }

   int MainWindow::WorkLoop() {
      MSG msg;
      for (;;) {
         const auto bRet = GetMessageW(&msg, nullptr, 0, 0);
         if (bRet > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
         } else if (bRet == 0) {
            return static_cast<int>(msg.wParam);
         } else {
            std::wstring wstrMessage = GetLastErrorStdWStr();
            MessageBoxW(nullptr, wstrMessage.c_str(), wszAppName, MB_ICONERROR);
            return 1;
         }
      }
   }

   std::unique_ptr<MainWindow> MainWindow::Create(HINSTANCE hInstance, int nShowCmd) {
      WNDCLASSEXW wc = {};
      wc.cbSize = sizeof(WNDCLASSEX);
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 0;
      wc.lpfnWndProc = MainWindow::_WndProc;
      wc.hInstance = hInstance;
      wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
      wc.lpszMenuName = nullptr;
      wc.lpszClassName = static_cast<LPCWSTR>(MainWindow::wszWndClass);

      if (RegisterClassExW(&wc) == 0) {
         std::wstring strMessage = GetLastErrorStdWStr();
         MessageBox(nullptr, strMessage.c_str(), wszAppName, MB_ICONERROR);
         return nullptr;
      }

      // Create the main window.
      auto pMainWindow = std::unique_ptr<MainWindow>(new MainWindow(hInstance, nShowCmd));
      HWND hWnd = CreateWindowExW(0,
                                  MainWindow::wszWndClass,
                                  wszAppName,
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  nullptr,
                                  nullptr,
                                  hInstance,
                                  pMainWindow.get());
      if (hWnd == nullptr) {
         std::wstring wstrMessage = GetLastErrorStdWStr();
         MessageBoxW(nullptr, wstrMessage.c_str(), wszAppName, MB_ICONERROR);
         return nullptr;
      } else {
         pMainWindow->hWnd = hWnd;
      }

      return pMainWindow;
   }

   void System::MainWindow::AddSizeChangeHandler(std::function<void(int, int)>&& handler) {
      resizeHandlers.push_back(std::move(handler));
   }
}
