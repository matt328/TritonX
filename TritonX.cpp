#include "pch.h"

#include "CMainWindow.h"

using namespace Microsoft::WRL;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd) {
   std::cout << "Hello World!\n";

     auto returnValue = 0;

   auto mainWindow = CMainWindow::Create(hInstance, nShowCmd);
   if (mainWindow) {
      returnValue = mainWindow->WorkLoop();
   }

   return returnValue;
}
