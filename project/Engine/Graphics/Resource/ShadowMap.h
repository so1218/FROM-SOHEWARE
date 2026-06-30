#pragma once

namespace FE
{

class SRVManager; 

class ShadowMap
{
public:

    // カスケード数を定義
    static const uint32_t kNumCascades = 4;

    // 初期化
    void Initialize(ID3D12Device* device, int width, int height, SRVManager* srvManager);

    // SRV用（メイン描画パスでテクスチャとして使うとき）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const;

    // パス用のヒープにコピーするためのCPUハンドル取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandleCPU() const;

    // DSV用（シャドウ生成パスで書き込み先として使うとき）
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(uint32_t cascadeIndex) const;

    // SRVのインデックス自体が必要になった時用
    uint32_t GetSRVIndex() const { return srvIndex_; }

    // リソースバリア（書き込みモード ⇄ 読み取りモード）
    void TransitionToDepthWrite(ID3D12GraphicsCommandList* commandList);
    void TransitionToRead(ID3D12GraphicsCommandList* commandList);

    void BeginPass(ID3D12GraphicsCommandList* commandList, uint32_t cascadeIndex);

    ID3D12Resource* GetResource() const { return shadowResource_.Get(); }


private:
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowResource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

    SRVManager* srvManager_ = nullptr;
    uint32_t srvIndex_ = 0; // SRVManagerから割り当てられたインデックス

    UINT width_ = 0;
    UINT height_ = 0;
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};

    uint32_t dsvDescriptorSize_ = 0;
};

}