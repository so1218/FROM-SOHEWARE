#pragma once
#include "Structures.h"
#include "IPostEffect.h"
#include "BrightExtractPass.h"
#include "BloomPass.h"
#include "DoFPass.h"
#include "FinalCompositePass.h"
#include "SSAOPass.h"
#include "BilateralBlurPass.h"
#include "VolumetricFogPass.h"
#include "VolumetricFogBilateralPass.h"
#include "Camera.h"

namespace FE
{

class Engine;
class SRVManager;
class PropertyBinder;

class PostEffectManager
{
public:
    ~PostEffectManager();

    // 初期化
    void Initialize(
        Engine* engine, uint32_t width, uint32_t height,
        RootSignatureManager* rootSigManager, PSOManager* psoManager,
        SRVManager* srvManager,
        uint32_t sceneDepthSrvIndex);

    // 更新処理
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition);

    // ポストエフェクト実行
    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);

    // 設定アクセス（ImGui用）
    BrightExtractSettings* GetBrightSettings() const { return brightPass_->GetSettings(); }
    BloomSettings* GetBloomSettings() const { return bloomPass_->GetSettings(); }
    FinalCompositeSettings* GetCompositeSettings() const { return compositePass_->GetSettings(); }
    DoFSettings* GetDoFSettings() const { return dofPass_->GetSettings(); }
    SSAOSettings* GetSSAOSettings() const { return ssaoPass_->GetSettings(); }
    BilateralBlurSettings* GetBilateralBlurSettings() const { return horizontalBilateralPass_->GetSettings(); }
    VolumetricFogSettings* GetVolumetricFogSettings() const { return volumetricFogPass_->GetSettings(); }
    FogBilateralSettings* GetFogBilateralSettings() const { return volumetricFogBilateralPass_->GetSettings(); }

    VolumetricFogPass* GetVolumetricFogPass() const { return volumetricFogPass_.get(); }

    // 出力リソース取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetBloomCombineSRVHandle() const { return compositePass_->GetSRVHandleGPU(); }
    PostEffectData* GetPostEffectData() const { return cbData_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return constantBuffer_->GetGPUVirtualAddress(); }

    uint32_t GetFinalCompositeSRVIndex() const { return compositePass_ ? compositePass_->GetSRVIndex() : 0; }
    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

    uint32_t GetVolumetricFogSRVIndex() const { return volumetricFogPass_->GetSRVIndex(); }

    const std::string& GetCurrentLutName() const { return currentLutName_; }
    void SetCurrentLutName(const std::string& name) { currentLutName_ = name; }

    void BeginFinalComposite(ID3D12GraphicsCommandList* cmdList);
    void EndFinalComposite(ID3D12GraphicsCommandList* cmdList);

    // シーン初期化時に PropertyBinder へパラメータを一括登録
    void BindProperties(PropertyBinder& binder, const std::string& prefix = "PostEffect");

    // ImGui でのパラメータ描画
    void DebugDraw(PropertyBinder& binder, const std::string& label = "ポストエフェクト");

private:
    // 依存オブジェクト
    Engine* engine_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_ = nullptr;

    // ポストエフェクトパス
    std::unique_ptr<BrightExtractPass> brightPass_;

    // Bloom用
    std::unique_ptr<BloomPass> bloomPass_;

    // DoF用
    std::unique_ptr<DoFPass> dofPass_;

    std::unique_ptr<FinalCompositePass> compositePass_;

    std::unique_ptr<SSAOPass> ssaoPass_;

    std::unique_ptr<BilateralBlurPass> horizontalBilateralPass_;
    std::unique_ptr<BilateralBlurPass> verticalBilateralPass_;

    std::unique_ptr<VolumetricFogPass> volumetricFogPass_;
    std::unique_ptr<VolumetricFogBilateralPass> volumetricFogBilateralPass_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    PostEffectData* cbData_ = nullptr;

    // シーン入力
    uint32_t sceneTextureIndex_ = 0;
    uint32_t sceneDepthIndex_ = 0;

    // 最終出力
    uint32_t finalPassSRVIndex_ = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE finalPassRTVHandle_;
    Microsoft::WRL::ComPtr<ID3D12Resource> finalPassResource_;

    // 光源のワールド座標を保存する変数
    Vector3 lightPosition_ = { 0, 100, 100 };

    std::string currentLutName_ = "LUT_Neutral_32";

    PostEffectContext context_;

    std::string prefix_;

    bool flagColorTint_ = false;
    bool flagVignette_ = false;
    bool flagChromAberration_ = false;
    bool flagPixelation_ = false;
    bool flagRadialBlur_ = false;
    bool flagScreenNoise_ = false;
    bool flagColorGradingLUT_ = false;
    bool flagSSAO_ = false;
    bool flagDoF_ = false;
    bool flagVolumetricFog_ = false;

    // 配置式フォグ個数管理用
    int fogVolumeCount_ = 0;
};

}