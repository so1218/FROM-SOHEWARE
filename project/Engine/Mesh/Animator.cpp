#include "Animator.h"

Animator:: Animator(
    const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    const ModelData& modelData,
    const Animation& animation,
    const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
    uint32_t descriptorSize,
    SRVAllocator* srvAllocator)
{
}

// アニメーションの更新処理を一つにまとめる
void Animator::Update(float deltaTime)
{
    // アニメーション時間を進める（ループ再生）
    animationTime_ += deltaTime;
    animationTime_ = fmod(animationTime_, currentAnimation_->duration);

    // 内部で関連する更新関数を呼び出す
    ApplyAnimation(skeleton_, *currentAnimation_, animationTime_);
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

// 描画に必要な情報を取得するゲッター
D3D12_GPU_DESCRIPTOR_HANDLE Animator::GetPaletteSrvHandle() const
{
    return skinCluster_.paletteSrvHandle.second;
}

const D3D12_VERTEX_BUFFER_VIEW& Animator::GetInfluenceBufferView() const
{
    return skinCluster_.influenceBufferView;
}