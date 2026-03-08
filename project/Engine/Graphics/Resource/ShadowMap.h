#pragma once

class SRVManager; 

class ShadowMap
{
public:
    // 初期化
    void Initialize(ID3D12Device* device, int width, int height, SRVManager* srvManager);

    // SRV用（メイン描画パスでテクスチャとして使うとき）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const;

    // DSV用（シャドウ生成パスで書き込み先として使うとき）
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;

    // リソースバリア（書き込みモード ⇄ 読み取りモード）
    void TransitionToDepthWrite(ID3D12GraphicsCommandList* commandList);
    void TransitionToRead(ID3D12GraphicsCommandList* commandList);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowResource_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

    SRVManager* srvManager_ = nullptr;
    uint32_t srvIndex_ = 0; // SRVManagerから割り当てられたインデックス
};