#pragma once

#include "pch.h"

namespace TX::System {
   class MainWindow {
    public:
      HINSTANCE GetHInstance() const noexcept {
         return hInstance;
      }

      HWND GetHwnd() const noexcept {
         return hWnd;
      }

      static std::unique_ptr<MainWindow> Create(HINSTANCE hInstance, int nShowCmd);

      void AddSizeChangeHandler(std::function<void(int, int)>&& handler);

      int WorkLoop();

    private:
      static constexpr WCHAR wszWndClass[] = L"MainWndClass";
      static constexpr WCHAR wszAppName[] = L"TritonX";

      HINSTANCE hInstance;
      HWND hWnd;
      int showCmd;

      std::vector<std::function<void(int, int)>> resizeHandlers;

      MainWindow(HINSTANCE hInstance, int nShowCmd) noexcept;
      static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      LRESULT OnCreate();
      LRESULT OnDestroy();
      LRESULT OnSize();
   };

   static std::wstring GetLastErrorStdWStr() {
      const auto error = ::GetLastError();
      if (error) {
         LPVOID lpMsgBuf = nullptr;
         DWORD bufLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                          FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr,
                                      error,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      reinterpret_cast<LPWSTR>(&lpMsgBuf),
                                      0,
                                      nullptr);
         if (bufLen) {
            LPWSTR lpMsgStr = static_cast<LPWSTR>(lpMsgBuf);
            std::wstring result(lpMsgStr, lpMsgStr + bufLen);
            ::LocalFree(lpMsgBuf);
            return result;
         }
      }
      return std::wstring();
   }

   template <class T, class U, HWND(U::*m_hWnd)>
   T* InstanceFromWndProc(HWND hWnd, UINT uMsg, LPARAM lParam) {
      T* windowInstance = nullptr;
      if (uMsg == WM_NCCREATE) {
         LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
         windowInstance = reinterpret_cast<T*>(pCreateStruct->lpCreateParams);
         SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowInstance));
         windowInstance->*m_hWnd = hWnd;
      } else {
         windowInstance = reinterpret_cast<T*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
      }

      return windowInstance;
   }
}
