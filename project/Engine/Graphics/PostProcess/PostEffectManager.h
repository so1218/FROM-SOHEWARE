#pragma once
#include "Structures.h"
#include "IPostEffect.h"
#include "BrightExtractPass.h"
#include "DownsamplePass.h"
#include "BlurPass.h"
#include "BokehBlurPass.h"
#include "BloomCombinePass.h"
#include "SSAOPass.h"
#include "BilateralBlurPass.h"
#include "SSRPass.h"
#include "VolumetricFogPass.h"
#include "VolumetricFogBilateralPass.h"
#include "Camera.h"

namespace FE
{

class Engine;
class SRVManager;

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
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition);

    // ポストエフェクト実行
    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);

    // 設定アクセス（ImGui用）
    BrightExtractSettings* GetBrightSettings() const { return brightPass_->GetSettings(); }
    BlurSettings* GetHorizontalBlurSettings() const { return horizontalBlurPass_->GetSettings(); }
    BlurSettings* GetVerticalBlurSettings() const { return verticalBlurPass_->GetSettings(); }
    CombineSettings* GetCombineSettings() const { return combinePass_->GetSettings(); }
    DoFSettings* GetDoFSettings() const { return bokehPass_->GetSettings(); }
    SSAOSettings* GetSSAOSettings() const { return ssaoPass_->GetSettings(); }
    BilateralBlurSettings* GetBilateralBlurSettings() const { return horizontalBilateralPass_->GetSettings(); }
    SSRSettings* GetSSRSettings() const { return ssrPass_->GetSettings(); }
    VolumetricFogSettings* GetVolumetricFogSettings() const { return volumetricFogPass_->GetSettings(); }
    FogBilateralSettings* GetFogBilateralSettings() const { return volumetricFogBilateralPass_->GetSettings(); }

    VolumetricFogPass* GetVolumetricFogPass() const { return volumetricFogPass_.get(); }

    // 出力リソース取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetBloomCombineSRVHandle() const { return combinePass_->GetSRVHandleGPU(); }
    PostEffectData* GetPostEffectData() const { return postEffectData_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return cbPostEffect_->GetGPUVirtualAddress(); }

    uint32_t GetBloomCombineSRVIndex() const { return combinePass_ ? combinePass_->GetSRVIndex() : 0; }
    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

    uint32_t GetVolumetricFogSRVIndex() const { return volumetricFogPass_->GetSRVIndex(); }

    const std::string& GetCurrentLutName() const { return currentLutName_; }
    void SetCurrentLutName(const std::string& name) { currentLutName_ = name; }

    const std::string& GetCurrentNoiseName() const { return currentNoiseName_; }
    void SetCurrentNoiseName(const std::string& name) { currentNoiseName_ = name; }

    void BeginFinalComposite(ID3D12GraphicsCommandList* cmdList);
    void EndFinalComposite(ID3D12GraphicsCommandList* cmdList);

    // 流体シミュレーションの結果を受け取る専用の窓口
    void SetFluidData(uint32_t densitySrvIndex, uint32_t velocitySrvIndex, uint32_t uvwSrvIndex, D3D12_GPU_VIRTUAL_ADDRESS cbAddress)
    {
        context_.fluidDensitySrvIndex = densitySrvIndex;
        context_.fluidVelocitySrvIndex = velocitySrvIndex;
        context_.fluidUVWSrvIndex = uvwSrvIndex; 
        context_.fluidSettingsCBAddress = cbAddress;
    }

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

    std::unique_ptr<SSAOPass> ssaoPass_;

    std::unique_ptr<SSRPass> ssrPass_;

    std::unique_ptr<BilateralBlurPass> horizontalBilateralPass_;
    std::unique_ptr<BilateralBlurPass> verticalBilateralPass_;

    std::unique_ptr<VolumetricFogPass> volumetricFogPass_;
    std::unique_ptr<VolumetricFogBilateralPass> volumetricFogBilateralPass_;

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

    std::string currentLutName_ = "LUT_Neutral_32";
    std::string currentNoiseName_ = "normal_00";

    PostEffectContext context_;
};

}