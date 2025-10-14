#pragma once

#include "AnimationData.h"

class Animator
{
public:
    // コンストラクタで関連オブジェクトを初期化
    Animator(
        const Microsoft::WRL::ComPtr<ID3D12Device>& device,
        const ModelData& modelData,
        const Animation& animation,
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
        uint32_t descriptorSize,
        SRVAllocator* srvAllocator);

    // アニメーションの更新処理を一つにまとめる
    void Update(float deltaTime);

    // 描画に必要な情報を取得するゲッター
    D3D12_GPU_DESCRIPTOR_HANDLE GetPaletteSrvHandle() const;
    const D3D12_VERTEX_BUFFER_VIEW& GetInfluenceBufferView() const;

private:
    Skeleton skeleton_;
    SkinCluster skinCluster_;
    const Animation* currentAnimation_;
    float animationTime_;
};