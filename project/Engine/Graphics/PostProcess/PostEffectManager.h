#pragma once
#include "Structures.h"
#include "IPostEffect.h"
#include "BrightExtractPass.h"
#include "DownsamplePass.h"
#include "BlurPass.h"
#include "BokehBlurPass.h"
#include "BloomCombinePass.h"
#include "GodRayPass.h"
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
        SRVManager* srvManager,
        uint32_t sceneDepthSrvIndex);

    // 更新処理
    void Update();

    // ポストエフェクト実行
    void ExecutePostEffects(
        ID3D12GraphicsCommandList* cmdListconst,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix,
        const Vector3& cameraPosition);

    // 設定アクセス（ImGui用）
    BrightExtractSettings* GetBrightSettings() const { return brightPass_->GetSettings(); }
    BlurSettings* GetHorizontalBlurSettings() const { return horizontalBlurPass_->GetSettings(); }
    BlurSettings* GetVerticalBlurSettings() const { return verticalBlurPass_->GetSettings(); }
    CombineSettings* GetCombineSettings() const { return combinePass_->GetSettings(); }
    GodRaySettings* GetGodRaySettings() const { return godRayPass_->GetSettings(); }

    // 出力リソース取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetBloomCombineSRVHandle() const { return combinePass_->GetSRVHandleGPU(); }
    PostEffectData* GetPostEffectData() const { return postEffectData_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return cbPostEffect_->GetGPUVirtualAddress(); }

    uint32_t GetBloomCombineSRVIndex() const { return combinePass_ ? combinePass_->GetSRVIndex() : 0; }
    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

    // 光源位置をセットする関数
    void SetLightPosition(const Vector3& pos) { lightPosition_ = pos; }

    // GodRay結果のSRVインデックスを取得する関数
    uint32_t GetGodRaySRVIndex() const { return godRayPass_->GetSRVIndex(); }

private:
    // 依存オブジェクト
    Engine* engine_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    RootSignatureManager* rootSigManager_ = nullptr;

    // ポストエフェクトパス
    std::unique_ptr<BrightExtractPass> brightPass_;

    // Bloom用
    std::unique_ptr<DownsamplePass> downsamplePass_;
    std::unique_ptr<BlurPass> verticalBlurPass_;
    std::unique_ptr<BlurPass> horizontalBlurPass_;

    // DoF用
    std::unique_ptr<BokehBlurPass> bokehPass_;

    std::unique_ptr<BloomCombinePass> combinePass_;

    std::unique_ptr<GodRayPass> godRayPass_;

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

    // 光源のワールド座標を保存する変数
    Vector3 lightPosition_ = { 0, 100, 100 };
};