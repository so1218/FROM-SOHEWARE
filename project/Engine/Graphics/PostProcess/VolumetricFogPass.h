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
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    // 外部からのアクセス用
    VolumetricFogSettings* GetSettings() { return cbData_; }

    // ノイズデータを受け取って保持する関数
    void SetNoiseData(const GeneratedTextureData& data) { noise3DData_ = data; }

private:
    // --- Froxel用の中間リソース ---
    // 1. 各セルの光と密度 (Injection用)
    Microsoft::WRL::ComPtr<ID3D12Resource> voxelInjectRes_;
    // 2. 蓄積された光と透過率 (Accumulation用)
    Microsoft::WRL::ComPtr<ID3D12Resource> voxelAccumulateRes_;

    // 各パス用のUAV/SRVインデックス（SRVManagerから取得したもの）
    uint32_t injectUavIndex_;
    uint32_t injectSrvIndex_;
    uint32_t accumUavIndex_;
    uint32_t accumSrvIndex_;

    // 設定用
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    VolumetricFogSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    GeneratedTextureData noise3DData_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

    // Froxelの解像度（画面の1/8〜1/16程度が一般的）
    const uint32_t froxelW = 160;
    const uint32_t froxelH = 90;
    const uint32_t froxelD = 64;

    // --- テンポラル用リソース ---
    // Resolveの結果を保存する2Dテクスチャ（2枚）
    Microsoft::WRL::ComPtr<ID3D12Resource> historyRes_[2];
    uint32_t historySrvIndices_[2];
    uint32_t historyUavIndices_[2];

    uint32_t frameCounter_ = 0; // フレーム入れ替え用
};

}