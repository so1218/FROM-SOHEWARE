#pragma once
#include "Structures.h"
#include "IPostEffect.h"
#include "BrightExtractPass.h"
#include "DownsamplePass.h"
#include "BlurPass.h"
#include "BloomCombinePass.h"
#include "Camera.h"
#include <memory>

class Engine;

class PostEffectManager
{
public:
    ~PostEffectManager();

    // 初期化
    void Initialize(
        Engine* engine, UINT width, UINT height,
        RootSignatureManager* rootSigManager, PSOManager* psoManager,
        Camera* camera, SRVManager* srvManager,
        uint32_t sceneDepthSrvIndex);

    // 更新
    void Update();

    // ポストエフェクト実行
    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);

    // ---- 設定アクセス（ImGui用） ----
    BrightExtractSettings* GetBrightSettings() const { return brightPass_->GetSettings(); }
    BlurSettings* GetHorizontalBlurSettings() const { return horizontalBlurPass_->GetSettings(); }
    BlurSettings* GetVerticalBlurSettings() const { return verticalBlurPass_->GetSettings(); }
    CombineSettings* GetCombineSettings() const { return combinePass_->GetSettings(); }

    // ---- 出力リソース ----
    D3D12_GPU_DESCRIPTOR_HANDLE GetBloomCombineSRVHandle() const { return combinePass_->GetSRVHandleGPU(); }

    PostEffectData* GetPostEffectData() const { return postEffectData_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return cbPostEffect_->GetGPUVirtualAddress(); }

    uint32_t GetBloomCombineSRVIndex() const
    {
        return combinePass_ ? combinePass_->GetSRVIndex() : 0;
    }

    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

private:
    // 依存オブジェクト
    Engine* engine_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    RootSignatureManager* rootSigManager_ = nullptr;

    // ---- ポストエフェクトパス ----
    std::unique_ptr<BrightExtractPass> brightPass_;

    // Bloom
    std::unique_ptr<DownsamplePass> downsamplePass_;
    std::unique_ptr<BlurPass> verticalBlurPass_;
    std::unique_ptr<BlurPass> horizontalBlurPass_;

    // DoF
    std::unique_ptr<DownsamplePass> dofDownsamplePass_;
    std::unique_ptr<BlurPass> dofVerticalBlurPass_;
    std::unique_ptr<BlurPass> dofHorizontalBlurPass_;

    std::unique_ptr<BloomCombinePass> combinePass_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> cbPostEffect_;
    PostEffectData* postEffectData_ = nullptr;

    // シーン入力
    uint32_t sceneTextureIndex_ = 0;
    uint32_t sceneDepthIndex_ = 0;

    // 最終出力
    uint32_t finalPassSRVIndex_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE finalPassRTVHandle_;
    Microsoft::WRL::ComPtr<ID3D12Resource> finalPassResource_;
};
