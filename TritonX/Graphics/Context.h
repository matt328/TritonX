#pragma once
namespace TX::Graphics {

   class Context {
    public:
      Context();

      void GetDefaultSize(int& width, int& height) const noexcept;
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

      Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
      Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[swapBufferCount];
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

      Microsoft::WRL::ComPtr<ID3D12Fence> fence;
      Microsoft::WRL::Wrappers::Event fenceEvent;

      std::array<UINT64, swapBufferCount> fenceValues;

      UINT rtvDescriptorSize = 0U;
      UINT backBufferIndex = 0U;

      D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

      void CreateDevice();
      void GetAdapter(IDXGIAdapter1** ppAdapter);
   };
}
