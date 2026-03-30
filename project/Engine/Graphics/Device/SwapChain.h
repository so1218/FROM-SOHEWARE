#pragma once

namespace FE
{

class SwapChain
{
public:
	void Initialize(
		HWND hwnd,
		ID3D12CommandQueue* commandQueue,
		uint32_t width,
		uint32_t height,
		uint32_t bufferCount,
		Microsoft::WRL::ComPtr <IDXGIFactory7> dxgiFactory);
	void Present();

	IDXGISwapChain4* GetSwapChain() const { return dxgiSwapChain_.Get(); }
	const DXGI_SWAP_CHAIN_DESC1& GetSwapChainDesc() const { return swapChainDesc_; }

private:
	Microsoft::WRL::ComPtr <IDXGISwapChain4> dxgiSwapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
};

}