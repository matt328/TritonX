#include "pch.h"

#include "System/MainWindow.h"
#include "Graphics/Context.h"

using namespace Microsoft::WRL;

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ const HINSTANCE hPrevInstance,
                    _In_ const PWSTR lpCmdLine,
                    _In_ int nShowCmd) {

   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   if (!DirectX::XMVerifyCPUSupport())
      return 1;

   Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
   if (FAILED(initialize))
      return 1;

   auto returnValue = 0;

   auto mainWindow = TX::System::MainWindow::Create(hInstance, nShowCmd);

   auto graphicsContext = std::make_unique<TX::Graphics::Context>(mainWindow->GetHwnd());

   if (mainWindow) {

      mainWindow->AddSizeChangeHandler([&graphicsContext](int width, int height) {
         graphicsContext->HandleSizeChange(width, height);
      });

      returnValue = mainWindow->WorkLoop();
   }

   return returnValue;
}
