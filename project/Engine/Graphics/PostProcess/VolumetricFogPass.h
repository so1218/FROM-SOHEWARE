#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"
#include "NoiseTextureGenerator.h"

namespace FE
{

class VolumetricFogPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    // IPostEffect
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    VolumetricFogSettings* GetSettings() { return cbData_; }

    // ノイズデータを受け取って保持する関数
    void SetNoiseData(const GeneratedTextureData& data) { noise3DData_ = data; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    VolumetricFogSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    // テクスチャリソースの寿命を管理
    GeneratedTextureData noise3DData_;
};

}