#pragma once

namespace FS
{

class CommandManager
{
public:
    ~CommandManager();

    void Initialize(ID3D12Device* device);

    // ゲッター
    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
    ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator_.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
    ID3D12Fence* GetFence() const { return fence_.Get(); }
    HANDLE GetFenceEvent() const { return fenceEvent_; }

    // フェンス・同期関連
    // コマンドキューに積まれた全ての処理が完了するまでCPUを待機させる
    void WaitForGPU();

    // 現在GPUが完了しているフェンスの値を取得
    uint64_t GetCompletedFenceValue() const { return fence_->GetCompletedValue(); }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

    // フェンス関連変数
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    HANDLE fenceEvent_ = nullptr;
    uint64_t fenceValue_ = 0;
};

}