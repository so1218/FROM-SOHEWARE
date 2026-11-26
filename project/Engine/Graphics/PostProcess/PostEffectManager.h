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

namespace FS
{

class Engine;
class SRVManager;
class PropertyBinder;

// 画面演出の、シーンに保存する設定
struct ScreenEffectSettings
{
    bool isVignetteEnabled = false;
    float vignetteAmount = 0.294f;
    float vignetteRadius = 0.148f;
    float vignetteSoftness = 0.3f;
    Vector2 vignetteEllipseScale = { 1.2f, 1.0f };
    Vector3 vignetteColor = { 1.0f, 1.0f, 1.0f };

    bool isChromaticAberrationEnabled = false;
    Vector2 chromaOffset = { 0.003f, 0.0f };

    bool isPixelationEnabled = false;
    float pixelationSize = 2.386f;

    bool isRadialBlurEnabled = false;
    float radialBlurStrength = 0.3f;
    Vector2 radialBlurCenter = { 0.5f, 0.5f };

    bool isScreenNoiseEnabled = false;
    float noiseAmount = 0.05f;
    float noiseSpeed = 1.0f;
    float noiseScale = 0.2f;
};

// ゲームから重ねる画面演出の値
struct ScreenEffectValues
{
    float radialBlurStrength = 0.0f;
    Vector2 radialBlurCenter = { 0.5f, 0.5f };

    // 色収差のずれ 
    Vector2 chromaOffset = { 0.0f, 0.0f };

    // 画面の縁を暗くする
    float vignetteAmount = 0.0f;
    Vector3 vignetteColor = { 0.0f, 0.0f, 0.0f };
    float vignetteStart = 0.25f;
    float vignetteEnd = 0.85f;

    // 1 より大きいときだけ、その大きさのドットにする
    float pixelationSize = 0.0f;
    // 砂嵐の量
    float noiseAmount = 0.0f;
};

// 決めた長さだけ重ねる画面演出
struct ScreenEffectRequest
{
    ScreenEffectValues values;

    // かけ始めから消えきるまでの長さ
    float duration = 0.5f;
    float fadeInTime = 0.0f;
    float fadeOutTime = 0.2f;
    // true なら時間の速さが変わっても、そのまま進める
    bool useUnscaledTime = true;

    // 揺れ
    // 1 秒に強弱を繰り返す回数
    float pulseFrequency = 0.0f;
    // どれだけ弱まるか
    float pulseDepth = 0.0f;
    // 不規則さ
    float pulseRandomness = 0.0f;
};

// PlayScreenEffect が返す番号
using ScreenEffectHandle = uint32_t;

class PostEffectManager
{
public:
    ~PostEffectManager();

    // 初期化
    void Initialize(
        Engine* engine,
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
    // シーンに保存する画面演出の設定
    ScreenEffectSettings& GetScreenEffectSettings() { return screenEffectSettings_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return constantBuffer_->GetGPUVirtualAddress(); }

    uint32_t GetFinalCompositeSRVIndex() const { return compositePass_ ? compositePass_->GetSRVIndex() : 0; }
    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

    void SetOverlayVignette(float amount, const Vector3& color, float start = 0.25f, float end = 0.85f);

    // このフレームだけ画面演出を重ねる
    void AddFrameScreenEffect(const ScreenEffectValues& values);

    // 画面演出を一時的に重ねる
    ScreenEffectHandle PlayScreenEffect(const ScreenEffectRequest& request);
    // フェードアウトの時間をかけて消す
    void StopScreenEffect(ScreenEffectHandle handle, bool isImmediate = false);
    // 重ねている演出を全部すぐに消す
    void ClearScreenEffects();
    bool IsScreenEffectPlaying(ScreenEffectHandle handle) const;

    uint32_t GetVolumetricFogSRVIndex() const { return volumetricFogPass_->GetSRVIndex(); }

    void BeginFinalComposite(ID3D12GraphicsCommandList* cmdList);
    void EndFinalComposite(ID3D12GraphicsCommandList* cmdList);

    // シーン初期化時に PropertyBinder へパラメータを一括登録
    void BindProperties(PropertyBinder& binder, const std::string& prefix = "PostEffect");

    // ImGui でのパラメータ描画
    void DebugGui(PropertyBinder& binder);
    void BindFogVolumes(PropertyBinder& binder);

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

    PostEffectContext context_;

    std::string prefix_;

    // 一時的に重ねている画面演出
    struct ActiveScreenEffect
    {
        ScreenEffectHandle handle = 0;
        ScreenEffectRequest request;
        float elapsedTime = 0.0f;
        // StopScreenEffect が呼ばれた時刻
        float stopTime = -1.0f;
    };

    // 重ねる演出の今の強さ
    static float CalculateScreenEffectWeight(const ActiveScreenEffect& effect);
    // 揺れの倍率
    static float CalculatePulse(const ScreenEffectRequest& request, float time);
    // 値に weight を掛けて、効果ごとに今の値より強いときだけ上書きする
    static void BlendScreenEffectValues(PostEffectData& data, const ScreenEffectValues& values, float weight);
    // 設定と重ねている演出を合わせて、GPU に渡す値を作る
    void ComposeScreenEffects();

    ScreenEffectSettings screenEffectSettings_;
    std::vector<ActiveScreenEffect> activeScreenEffects_;
    // AddFrameScreenEffect で渡された、このフレームの分
    std::vector<ScreenEffectValues> frameScreenEffects_;
    ScreenEffectHandle nextScreenEffectHandle_ = 1;
    Vector2 screenResolution_ = { 0.0f, 0.0f };

    // SetOverlayVignette で渡された値
    float overlayVignetteAmount_ = 0.0f;
    Vector3 overlayVignetteColor_ = { 0.0f, 0.0f, 0.0f };
    float overlayVignetteStart_ = 0.25f;
    float overlayVignetteEnd_ = 0.85f;
    bool flagSSAO_ = false;
    bool flagDoF_ = false;
    bool flagVolumetricFog_ = false;

    // 配置式フォグ個数管理用
    int fogVolumeCount_ = 0;
};

}