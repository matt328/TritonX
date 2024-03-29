#pragma once

#include "StepTimer.h"

namespace TX::Graphics {

   class Context {
    public:
      Context();
      ~Context();

      void GetDefaultSize(int& width, int& height) const noexcept;

      void CaptureState() noexcept;
      void GetState(RECT& rect) const noexcept;

      void HandleSizeChange(int width, int height) noexcept;

      void Initialize(HWND window, int width, int height);
      void Tick();

      // Messages
      void OnActivated();
      void OnDeactivated();
      void OnSuspending();
      void OnResuming();
      void OnWindowSizeChanged(int width, int height);

    private:
      static const UINT swapBufferCount = 2;

      HWND window;
      int outputWidth;
      int outputHeight;
      /// <summary>
      /// Only use for fullscreen transitions
      /// </summary>
      RECT prevRect;

      Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
      Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

      std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, swapBufferCount> commandAllocators;
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

      Microsoft::WRL::ComPtr<ID3D12Fence> fence;
      Microsoft::WRL::Wrappers::Event fenceEvent;

      std::array<UINT64, swapBufferCount> fenceValues;

      Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
      std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, swapBufferCount> renderTargets;
      Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil;

      StepTimer timer;

      UINT rtvDescriptorSize;
      UINT backBufferIndex;

      D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

      void CreateDevice();
      void GetAdapter(IDXGIAdapter1** ppAdapter);
      void CreateResources();
      void WaitForGpu() noexcept;

      void OnDeviceLost();

      void Update(StepTimer const& timer);
      void Render();
      void Clear();
      void Present();
      void MoveToNextFrame();
   };
}
