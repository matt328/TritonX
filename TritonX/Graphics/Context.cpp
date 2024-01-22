#include "pch.h"

#include "Context.h"
#include "Helpers.h"

namespace TX::Graphics {

   using Microsoft::WRL::ComPtr;

   Context::Context(HWND hWnd) {
      CreateDevice();
   }

   void Context::HandleSizeChange(int width, int height) noexcept {
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

}
