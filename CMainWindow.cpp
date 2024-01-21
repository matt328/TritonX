#include "pch.h"

#include "CMainWindow.h"

CMainWindow::CMainWindow(HINSTANCE hInstance, int nShowCmd) :
    hInstance(hInstance), showCmd(nShowCmd) {
}

LRESULT CALLBACK CMainWindow::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   CMainWindow* pMainWindow =
       InstanceFromWndProc<CMainWindow, CMainWindow, &CMainWindow::hWnd>(hWnd, uMsg, lParam);

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
CMainWindow::OnCreate() {
   SetWindowPos(hWnd, nullptr, 0, 0, 800, 600, SWP_NOMOVE);
   // Finally, show the window.
   ShowWindow(hWnd, showCmd);
   return 0;
}

LRESULT
CMainWindow::OnDestroy() {
   PostQuitMessage(0);
   return 0;
}

LRESULT
CMainWindow::OnSize() {
   RECT rcWindow;
   GetClientRect(hWnd, &rcWindow);
   return 0;
}

int CMainWindow::WorkLoop() {
   MSG msg;
   for (;;) {
      BOOL bRet = GetMessageW(&msg, 0, 0, 0);
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

std::unique_ptr<CMainWindow> CMainWindow::Create(HINSTANCE hInstance, int nShowCmd) {
   WNDCLASSW wc = {};
   wc.lpfnWndProc = CMainWindow::_WndProc;
   wc.hInstance = hInstance;
   wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
   wc.lpszClassName = CMainWindow::wszWndClass;

   if (RegisterClassW(&wc) == 0) {
      std::wstring strMessage = GetLastErrorStdWStr();
      MessageBox(nullptr, strMessage.c_str(), wszAppName, MB_ICONERROR);
      return nullptr;
   }

   // Create the main window.
   auto pMainWindow = std::unique_ptr<CMainWindow>(new CMainWindow(hInstance, nShowCmd));
   HWND hWnd = CreateWindowExW(0,
                               CMainWindow::wszWndClass,
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
