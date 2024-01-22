#pragma once
namespace TX::Graphics {

   class Context {
    public:
      Context(HWND hWnd);

      void HandleSizeChange(int width, int height) noexcept;

    private:
      static const UINT swapBufferCount = 2;
      Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
      Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

      Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

      Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[swapBufferCount];
      Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

      Microsoft::WRL::ComPtr<ID3D12Fence> fence;
      Microsoft::WRL::Wrappers::Event fenceEvent;

      UINT64 fenceValues[swapBufferCount];

      UINT rtvDescriptorSize = 0U;
      UINT backBufferIndex = 0U;

      D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

      void CreateDevice();
      void GetAdapter(IDXGIAdapter1** ppAdapter);
   };
}
