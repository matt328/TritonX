#include "pch.h"

#include "Context.h"
#include "Helpers.h"

namespace TX::Graphics {

   using Microsoft::WRL::ComPtr;

   Context::Context() :
       fenceValues{}, outputHeight(0), outputWidth(0), prevRect({}), window(nullptr),
       rtvDescriptorSize(0), backBufferIndex(0), renderTargets{} {
   }

   Context::~Context() {
      WaitForGpu();
   }

   void Context::GetDefaultSize(int& width, int& height) const noexcept {
      width = 1280;
      height = 720;
   }

   void Context::CaptureState() noexcept {
      GetWindowRect(window, &prevRect);
   }

   void Context::GetState(RECT& rect) const noexcept {
      rect = prevRect;
   }

   void Context::HandleSizeChange(int width, int height) noexcept {
   }

   void Context::Initialize(HWND window, int width, int height) {
      this->window = window;
      outputWidth = width;
      outputHeight = height;

      CreateDevice();
      CreateResources();

      timer.SetFixedTimeStep(true);
      timer.SetTargetElapsedSeconds(1.0 / 240);
   }

   void Context::Tick() {
      timer.Tick([&]() { Update(timer); });
      Render();
   }

   void Context::Update(StepTimer const& timer) {
      float elapsedTime = float(timer.GetElapsedSeconds());

      elapsedTime;
   }

   void Context::Render() {
      if (timer.GetFrameCount() == 0) {
         return;
      }

      Clear();

      Present();
   }

   void Context::Clear() {
      // Reset Command List and Allocator
      ThrowIfFailed(commandAllocators[backBufferIndex]->Reset());
      ThrowIfFailed(commandList->Reset(commandAllocators[backBufferIndex].Get(), nullptr));

      // Transition render target into correct state
      const D3D12_RESOURCE_BARRIER barrier =
          CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
                                               D3D12_RESOURCE_STATE_PRESENT,
                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
      commandList->ResourceBarrier(1, &barrier);

      // Clear the Views
      auto cpuHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
      auto cpuHandleDsv = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

      const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
          cpuHandle, static_cast<INT>(backBufferIndex), rtvDescriptorSize);
      CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptor(cpuHandleDsv);
      commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
      commandList->ClearRenderTargetView(
          rtvDescriptor, DirectX::Colors::CornflowerBlue, 0, nullptr);
      commandList->ClearDepthStencilView(
          dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

      // Set the viewport and scissor rect.
      const D3D12_VIEWPORT viewport = {0.0f,
                                       0.0f,
                                       static_cast<float>(outputWidth),
                                       static_cast<float>(outputHeight),
                                       D3D12_MIN_DEPTH,
                                       D3D12_MAX_DEPTH};
      const D3D12_RECT scissorRect = {
          0, 0, static_cast<LONG>(outputWidth), static_cast<LONG>(outputHeight)};
      commandList->RSSetViewports(1, &viewport);
      commandList->RSSetScissorRects(1, &scissorRect);
   }

   void Context::Present() {
      // Transition the render target to the state that allows it to be presented to the display.
      const D3D12_RESOURCE_BARRIER barrier =
          CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].Get(),
                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                               D3D12_RESOURCE_STATE_PRESENT);
      commandList->ResourceBarrier(1, &barrier);

      // Send the command list off to the GPU for processing.
      ThrowIfFailed(commandList->Close());
      commandQueue->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

      // The first argument instructs DXGI to block until VSync, putting the application
      // to sleep until the next VSync. This ensures we don't waste any cycles rendering
      // frames that will never be displayed to the screen.
      HRESULT hr = swapChain->Present(1, 0);

      // If the device was reset we must completely reinitialize the renderer.
      if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
         OnDeviceLost();
      } else {
         ThrowIfFailed(hr);
         MoveToNextFrame();
      }
   }

   void Context::MoveToNextFrame() {
      // Schedule a Signal command in the queue.
      const UINT64 currentFenceValue = fenceValues[backBufferIndex];
      ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

      // Update the back buffer index.
      backBufferIndex = swapChain->GetCurrentBackBufferIndex();

      // If the next frame is not ready to be rendered yet, wait until it is ready.
      if (fence->GetCompletedValue() < fenceValues[backBufferIndex]) {
         ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[backBufferIndex], fenceEvent.Get()));
         std::ignore = WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
      }

      // Set the fence value for the next frame.
      fenceValues[backBufferIndex] = currentFenceValue + 1;
   }

   void Context::WaitForGpu() noexcept {
      if (commandQueue && fence && fenceEvent.IsValid()) {
         auto fenceValue = fenceValues[backBufferIndex];
         if (SUCCEEDED(commandQueue->Signal(fence.Get(), fenceValue))) {
            if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent.Get()))) {
               std::ignore = WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
               fenceValues[backBufferIndex]++;
            }
         }
      }
   }

   void Context::CreateResources() {
      WaitForGpu();

      for (UINT n = 0; n < swapBufferCount; n++) {
         renderTargets[n].Reset();
         fenceValues[n] = fenceValues[backBufferIndex];
      }

      constexpr DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
      constexpr DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
      const UINT backBufferWidth = static_cast<UINT>(outputWidth);
      const UINT backBufferHeight = static_cast<UINT>(outputHeight);

      if (swapChain) {
         HRESULT hr = swapChain->ResizeBuffers(
             swapBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);
         if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            OnDeviceLost();
            return;
         } else {
            ThrowIfFailed(hr);
         }
      } else {
         DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
         swapChainDesc.Width = backBufferWidth;
         swapChainDesc.Height = backBufferHeight;
         swapChainDesc.Format = backBufferFormat;
         swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
         swapChainDesc.BufferCount = swapBufferCount;
         swapChainDesc.SampleDesc.Count = 1;
         swapChainDesc.SampleDesc.Quality = 0;
         swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
         swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
         swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

         DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
         fsSwapChainDesc.Windowed = TRUE;

         ComPtr<IDXGISwapChain1> swapChain1;
         ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(),
                                                           window,
                                                           &swapChainDesc,
                                                           &fsSwapChainDesc,
                                                           nullptr,
                                                           swapChain1.GetAddressOf()));
         ThrowIfFailed(swapChain1.As(&swapChain));
         ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
      }

      // Create rtvDescriptors
      for (UINT n = 0; n < swapBufferCount; n++) {
         ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(renderTargets[n].GetAddressOf())));

         wchar_t name[25] = {};
         swprintf_s(name, L"Render Target %u", n);
         renderTargets[n]->SetName(name);

         auto cpuHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

         const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
             cpuHandle, static_cast<INT>(n), rtvDescriptorSize);
         d3dDevice->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvDescriptor);
      }

      backBufferIndex = swapChain->GetCurrentBackBufferIndex();

      const CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

      D3D12_RESOURCE_DESC depthStencilDesc =
          CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1);

      depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

      const CD3DX12_CLEAR_VALUE depthOptimizedClearValue(depthBufferFormat, 1.0f, 0u);

      ThrowIfFailed(
          d3dDevice->CreateCommittedResource(&depthHeapProperties,
                                             D3D12_HEAP_FLAG_NONE,
                                             &depthStencilDesc,
                                             D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                             &depthOptimizedClearValue,
                                             IID_PPV_ARGS(depthStencil.ReleaseAndGetAddressOf())));
      depthStencil->SetName(L"Depth Stencil");

      D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
      dsvDesc.Format = depthBufferFormat;
      dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

      auto cpuHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

      d3dDevice->CreateDepthStencilView(depthStencil.Get(), &dsvDesc, cpuHandle);
   }

   void Context::OnDeviceLost() {
   }

   void Context::CreateDevice() {
      DWORD dxgiFactoryFlags = 0;

#if defined(_DEBUG)
      {
         ComPtr<ID3D12Debug> debugController;
         if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
            debugController->EnableDebugLayer();
         }

         ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
         if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
            dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(
                DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(
                DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
         }
      }
#endif // _DEBUG

      ThrowIfFailed(
          CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

      ComPtr<IDXGIAdapter1> adapter;
      GetAdapter(adapter.GetAddressOf());

      // Create the D3D Device Finally
      ThrowIfFailed(D3D12CreateDevice(
          adapter.Get(), featureLevel, IID_PPV_ARGS(d3dDevice.ReleaseAndGetAddressOf())));

      // Set up some Device specific Debug things
      // Guess NDEBUG differs from _DEBUG
#ifndef NDEBUG
      ComPtr<ID3D12InfoQueue> d3dInfoQueue;
      if (SUCCEEDED(d3dDevice.As(&d3dInfoQueue))) {
#ifdef _DEBUG
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
         auto hide = std::array<D3D12_MESSAGE_ID, 4>{
             D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
             D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
             // Workarounds for debug layer issues on hybrid-graphics systems
             D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
             D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
         };
         D3D12_INFO_QUEUE_FILTER filter = {};
         filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
         filter.DenyList.pIDList = hide.data();
         d3dInfoQueue->AddStorageFilterEntries(&filter);
      }
#endif

      // Create the Command Queue
      auto queueDesc = D3D12_COMMAND_QUEUE_DESC{.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE};

      ThrowIfFailed(d3dDevice->CreateCommandQueue(
          &queueDesc, IID_PPV_ARGS(commandQueue.ReleaseAndGetAddressOf())));

      // Create Descriptor Heaps for RTV and DSV
      auto rtvDescriptorHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
          .NumDescriptors = swapBufferCount,
      };

      auto dsvDescriptorHeapDesc = D3D12_DESCRIPTOR_HEAP_DESC{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
          .NumDescriptors = 1U,
      };

      ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
          &rtvDescriptorHeapDesc, IID_PPV_ARGS(rtvDescriptorHeap.ReleaseAndGetAddressOf())));

      ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
          &dsvDescriptorHeapDesc, IID_PPV_ARGS(dsvDescriptorHeap.ReleaseAndGetAddressOf())));

      rtvDescriptorSize =
          d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

      // Create a Command Allocator for each back buffer
      for (UINT n = 0; n < swapBufferCount; n++) {
         ThrowIfFailed(d3dDevice->CreateCommandAllocator(
             D3D12_COMMAND_LIST_TYPE_DIRECT,
             IID_PPV_ARGS(commandAllocators[n].ReleaseAndGetAddressOf())));
      }

      // Create a command list for recording graphics commands
      ThrowIfFailed(
          d3dDevice->CreateCommandList(0,
                                       D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       commandAllocators[0].Get(),
                                       nullptr,
                                       IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf())));

      ThrowIfFailed(commandList->Close());

      // Create a fence for tracking GPU execution
      ThrowIfFailed(d3dDevice->CreateFence(fenceValues[backBufferIndex],
                                           D3D12_FENCE_FLAG_NONE,
                                           IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
      fenceValues[backBufferIndex]++;

      fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));

      if (!fenceEvent.IsValid()) {
         throw std::system_error(
             std::error_code(static_cast<int>(GetLastError()), std::system_category()),
             "CreateEventEx");
      }

      // Check Shader Model 6 support
      D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_SHADER_MODEL_6_0};
      if (FAILED(d3dDevice->CheckFeatureSupport(
              D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))) {
#ifdef _DEBUG
         OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
         throw std::runtime_error("Shader Model 6.0 is not supported!");
      }
   }

   void Context::OnActivated() {
   }

   void Context::GetAdapter(IDXGIAdapter1** ppAdapter) {
      *ppAdapter = nullptr;

      ComPtr<IDXGIAdapter1> adapter;
      for (UINT adapterIndex = 0;
           DXGI_ERROR_NOT_FOUND !=
           dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());
           ++adapterIndex) {

         DXGI_ADAPTER_DESC1 desc;

         ThrowIfFailed(adapter->GetDesc1(&desc));

         if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // naw.
            continue;
         }

         if (SUCCEEDED(
                 D3D12CreateDevice(adapter.Get(), featureLevel, __uuidof(ID3D12Device), nullptr))) {
            break;
         }
      }

#if !defined(NDEBUG) // Soo if DEBUG
      if (!adapter) {
         if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())))) {
            throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional "
                                     "feature? whatever that means.");
         }
      }
#endif
      if (!adapter) {
         throw std::runtime_error(
             "No Direct3D 12 device found. You will not go to space today. :(");
      }

      *ppAdapter = adapter.Detach();
   }

   void Context::OnDeactivated() {
   }

   void Context::OnSuspending() {
   }

   void Context::OnResuming() {
   }

   void Context::OnWindowSizeChanged(int width, int height) {
      if (!window) {
         return;
      }

      outputWidth = width;
      outputHeight = height;
   }

}
