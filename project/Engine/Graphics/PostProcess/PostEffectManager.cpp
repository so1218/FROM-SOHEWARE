#include "pch.h"
#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"
#include "GraphicsDevice.h"
#include "RTVManager.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "SRVManager.h"
#include "PropertyBinder.h"
#include "DebugDraw.h"
#include "Frustum.h"

namespace FS
{

PostEffectManager::~PostEffectManager()
{
}

void PostEffectManager::Initialize(
    Engine* engine,
    RootSignatureManager* rootSigManager,
    PSOManager* psoManager,
    SRVManager* srvManager,
    uint32_t sceneDepthSrvIndex)
{
    engine_ = engine;
    srvManager_ = srvManager;
    rootSignatureManager_ = rootSigManager;

    // シーンカラー / 深度SRV
    sceneTextureIndex_ = engine->GetOffscreenRTVManager()->GetOffscreenSRVIndex(static_cast<uint32_t>(GBufferIndex::Color));
    sceneDepthIndex_ = sceneDepthSrvIndex;

	uint32_t width = Engine::GetClientWidth(); 
    uint32_t height = Engine::GetClientHeight();

    // 輝度抽出
    brightPass_ = std::make_unique<BrightExtractPass>();
    brightPass_->Initialize(engine, width, height, psoManager);

    // Bloom
    bloomPass_ = std::make_unique<BloomPass>();
    bloomPass_->Initialize(engine, width, height, psoManager);

    // DOF(Bokeh)の初期化
    uint32_t halfW = Math::MyMax(1u, width / 2);
    uint32_t halfH = Math::MyMax(1u, height / 2);

    dofPass_ = std::make_unique<DoFPass>();
    dofPass_->Initialize(engine, halfW, halfH, psoManager);

    // 最終合成
    compositePass_ = std::make_unique<FinalCompositePass>();
    compositePass_->Initialize(engine, width, height, psoManager, srvManager);

    // SSAO初期化
    ssaoPass_ = std::make_unique<SSAOPass>();
    ssaoPass_->Initialize(engine, halfW, halfH, psoManager);

    // BilateralBlur初期化 (横)
    horizontalBilateralPass_ = std::make_unique<BilateralBlurPass>();
    horizontalBilateralPass_->Initialize(engine, halfW, halfH, psoManager);
    horizontalBilateralPass_->GetSettings()->direction = { 1.0f, 0.0f };

    // BilateralBlur初期化 (縦)
    verticalBilateralPass_ = std::make_unique<BilateralBlurPass>();
    verticalBilateralPass_->Initialize(engine, halfW, halfH, psoManager);
    verticalBilateralPass_->GetSettings()->direction = { 0.0f, 1.0f };

    // VolumetricFog初期化
    uint32_t volFogW = Math::MyMax(1u, width / 2);
    uint32_t volFogH = Math::MyMax(1u, height / 2);
    volumetricFogPass_ = std::make_unique<VolumetricFogPass>();
    volumetricFogPass_->Initialize(engine, volFogW, volFogH, psoManager);

    volumetricFogBilateralPass_ = std::make_unique<VolumetricFogBilateralPass>();
    volumetricFogBilateralPass_->Initialize(engine, volFogW, volFogH, psoManager);

    // ポストエフェクト定数バッファ
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<PostEffectData>(
        device,
        &cbData_
    );

    // 最終出力用オフスクリーンRT（Create後にSRVIndexが更新される）
    auto [finalResource, finalRtvHandle, finalSrvIndex, uavIndex] =
        engine_->GetOffscreenRTVManager()->CreateOffscreenRenderTarget(
            width, height, Vector4(0, 0, 0, 1), DXGI_FORMAT_R16G16B16A16_FLOAT
        );

    finalPassResource_ = finalResource;
    finalPassRTVHandle_ = finalRtvHandle;
    finalPassSRVIndex_ = finalSrvIndex;

    // GPU に渡す値を一度作っておく
    screenResolution_ = Vector2(float(width), float(height));
    ComposeScreenEffects();
}

void PostEffectManager::SetOverlayVignette(float amount, const Vector3& color, float start, float end)
{
    overlayVignetteAmount_ = std::clamp(amount, 0.0f, 1.0f);
    overlayVignetteColor_ = color;
    overlayVignetteStart_ = std::max(start, 0.0f);
    overlayVignetteEnd_ = std::max(end, overlayVignetteStart_);
}

void PostEffectManager::AddFrameScreenEffect(const ScreenEffectValues& values)
{
    frameScreenEffects_.push_back(values);
}

ScreenEffectHandle PostEffectManager::PlayScreenEffect(const ScreenEffectRequest& request)
{
    ActiveScreenEffect effect;
    effect.handle = nextScreenEffectHandle_++;
    // 一周して 0 にならないようにする
    if (nextScreenEffectHandle_ == 0)
    {
        nextScreenEffectHandle_ = 1;
    }
    effect.request = request;
    activeScreenEffects_.push_back(effect);
    return effect.handle;
}

void PostEffectManager::StopScreenEffect(ScreenEffectHandle handle, bool isImmediate)
{
    for (auto it = activeScreenEffects_.begin(); it != activeScreenEffects_.end(); ++it)
    {
        if (it->handle != handle)
        {
            continue;
        }
        if (isImmediate || it->request.fadeOutTime <= 0.0f)
        {
            activeScreenEffects_.erase(it);
        }
        else if (it->stopTime < 0.0f)
        {
            it->stopTime = it->elapsedTime;
        }
        return;
    }
}

void PostEffectManager::ClearScreenEffects()
{
    activeScreenEffects_.clear();
    frameScreenEffects_.clear();
}

bool PostEffectManager::IsScreenEffectPlaying(ScreenEffectHandle handle) const
{
    return std::any_of(activeScreenEffects_.begin(), activeScreenEffects_.end(),
        [handle](const ActiveScreenEffect& effect) { return effect.handle == handle; });
}

float PostEffectManager::CalculateScreenEffectWeight(const ActiveScreenEffect& effect)
{
    const ScreenEffectRequest& request = effect.request;
    const float time = effect.elapsedTime;

    const float fadeInWeight = request.fadeInTime > 0.0f ? std::min(time / request.fadeInTime, 1.0f) : 1.0f;

    // 止められたときは、その時刻からフェードアウトする
    float fadeOutStart = std::numeric_limits<float>::max();
    if (effect.stopTime >= 0.0f)
    {
        fadeOutStart = effect.stopTime;
    }
    else if (request.duration > 0.0f)
    {
        fadeOutStart = request.duration - std::max(request.fadeOutTime, 0.0f);
    }

    float fadeOutWeight = 1.0f;
    if (time >= fadeOutStart)
    {
        if (request.fadeOutTime <= 0.0f)
        {
            return -1.0f;
        }
        fadeOutWeight = 1.0f - (time - fadeOutStart) / request.fadeOutTime;
        if (fadeOutWeight <= 0.0f)
        {
            return -1.0f;
        }
    }
    return std::min(fadeInWeight, fadeOutWeight) * CalculatePulse(request, time);
}

float PostEffectManager::CalculatePulse(const ScreenEffectRequest& request, float time)
{
    const float depth = std::clamp(request.pulseDepth, 0.0f, 1.0f);
    if (request.pulseFrequency <= 0.0f || depth <= 0.0f)
    {
        return 1.0f;
    }

    // 規則正しい揺れ
    const float phase = time * request.pulseFrequency;
    const float wave = 0.5f + 0.5f * std::cos(phase * 2.0f * std::numbers::pi_v<float>);

    // 不規則な揺れ
    auto hash = [](float value)
        {
            const float result = std::sin(value * 12.9898f) * 43758.5453f;
            return result - std::floor(result);
        };
    const float noisePosition = phase * 2.0f;
    const float noiseIndex = std::floor(noisePosition);
    float noiseRatio = noisePosition - noiseIndex;
    noiseRatio = noiseRatio * noiseRatio * (3.0f - 2.0f * noiseRatio);
    const float noise = hash(noiseIndex) + (hash(noiseIndex + 1.0f) - hash(noiseIndex)) * noiseRatio;

    const float randomness = std::clamp(request.pulseRandomness, 0.0f, 1.0f);
    const float strength = wave + (noise - wave) * randomness;
    return 1.0f - depth * (1.0f - strength);
}

void PostEffectManager::BlendScreenEffectValues(PostEffectData& data, const ScreenEffectValues& values, float weight)
{
    if (weight <= 0.0f)
    {
        return;
    }

    const float radialBlurStrength = values.radialBlurStrength * weight;
    const float currentRadialBlur = (data.flag & RADIAL_BLUR) ? data.radialBlurStrength : 0.0f;
    if (radialBlurStrength > currentRadialBlur)
    {
        data.radialBlurStrength = radialBlurStrength;
        data.radialBlurCenter = values.radialBlurCenter;
        data.flag |= RADIAL_BLUR;
    }

    const Vector2 chromaOffset = values.chromaOffset * weight;
    const float currentChroma = (data.flag & CHROM_ABERRATION) ? data.chromaOffset.Length() : 0.0f;
    if (chromaOffset.Length() > currentChroma)
    {
        data.chromaOffset = chromaOffset;
        data.flag |= CHROM_ABERRATION;
    }

    if (values.pixelationSize > 1.0f)
    {
        const float pixelationSize = 1.0f + (values.pixelationSize - 1.0f) * weight;
        const float currentPixelation = (data.flag & PIXELATION) ? data.pixelationSize : 1.0f;
        if (pixelationSize > currentPixelation)
        {
            data.pixelationSize = pixelationSize;
            data.flag |= PIXELATION;
        }
    }

    const float noiseAmount = values.noiseAmount * weight;
    const float currentNoise = (data.flag & SCREEN_NOISE) ? data.noiseAmount : 0.0f;
    if (noiseAmount > currentNoise)
    {
        data.noiseAmount = noiseAmount;
        data.flag |= SCREEN_NOISE;
    }

    const float vignetteAmount = std::clamp(values.vignetteAmount, 0.0f, 1.0f) * weight;
    if (vignetteAmount > data.overlayVignetteAmount)
    {
        data.overlayVignetteAmount = vignetteAmount;
        data.overlayVignetteColor = values.vignetteColor;
        data.overlayVignetteStart = std::max(values.vignetteStart, 0.0f);
        data.overlayVignetteEnd = std::max(values.vignetteEnd, data.overlayVignetteStart);
    }
}

void PostEffectManager::ComposeScreenEffects()
{
    if (!cbData_)
    {
        return;
    }

    // シーンの設定を土台にする
    const ScreenEffectSettings& settings = screenEffectSettings_;
    PostEffectData data = {};
    data.totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    data.screenResolution = screenResolution_;
    data.vignetteAmount = settings.vignetteAmount;
    data.vignetteRadius = settings.vignetteRadius;
    data.vignetteSoftness = settings.vignetteSoftness;
    data.vignetteEllipseScale = settings.vignetteEllipseScale;
    data.vignetteColor = settings.vignetteColor;
    data.chromaOffset = settings.chromaOffset;
    data.pixelationSize = settings.pixelationSize;
    data.radialBlurStrength = settings.radialBlurStrength;
    data.radialBlurCenter = settings.radialBlurCenter;
    data.noiseAmount = settings.noiseAmount;
    data.noiseSpeed = settings.noiseSpeed;
    data.noiseScale = settings.noiseScale;
    data.overlayVignetteAmount = overlayVignetteAmount_;
    data.overlayVignetteColor = overlayVignetteColor_;
    data.overlayVignetteStart = overlayVignetteStart_;
    data.overlayVignetteEnd = overlayVignetteEnd_;

    data.flag = 0;
    if (settings.isVignetteEnabled)            data.flag |= VIGNETTE;
    if (settings.isChromaticAberrationEnabled) data.flag |= CHROM_ABERRATION;
    if (settings.isPixelationEnabled)          data.flag |= PIXELATION;
    if (settings.isRadialBlurEnabled)          data.flag |= RADIAL_BLUR;
    if (settings.isScreenNoiseEnabled)         data.flag |= SCREEN_NOISE;

    // 重ねる演出は、効果ごとに土台より強いときだけ上書き
    for (const ActiveScreenEffect& effect : activeScreenEffects_)
    {
        BlendScreenEffectValues(data, effect.request.values, CalculateScreenEffectWeight(effect));
    }
    for (const ScreenEffectValues& values : frameScreenEffects_)
    {
        BlendScreenEffectValues(data, values, 1.0f);
    }
    // このフレームの分は使い終わったので捨てる
    frameScreenEffects_.clear();

    // 作った値をまとめて書き込むだけにする
    *cbData_ = data;
}

void PostEffectManager::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition)
{
    // 重ねている画面演出の時間を進め、消えきったものを外す
    {
        TimeManager* timeManager = TimeManager::GetInstance();
        const float deltaTime = timeManager->GetDeltaTime();
        const float unscaledDeltaTime = timeManager->GetUnscaledDeltaTime();
        for (ActiveScreenEffect& effect : activeScreenEffects_)
        {
            effect.elapsedTime += effect.request.useUnscaledTime ? unscaledDeltaTime : deltaTime;
        }
        std::erase_if(activeScreenEffects_,
            [](const ActiveScreenEffect& effect) { return CalculateScreenEffectWeight(effect) < 0.0f; });
    }

    if (volumetricFogPass_)
    {
        volumetricFogPass_->BeginFrame();

        // 映っていない置いた霧は送らない。
        constexpr float kSelfShadowReach = 15.0f;
        Frustum frustum;
        frustum.ExtractFromMatrix(viewMatrix * projectionMatrix);
        const float fogMaxDistance = volumetricFogPass_->GetSettings()->maxDistance;

        for (const auto& volume : volumetricFogPass_->GetFogVolumesData())
        {
            // 球は scale.x が半径
            float radius = (volume.type == 0) ? volume.scale.x : volume.scale.Length();
            radius += kSelfShadowReach;

            if ((volume.position - cameraPosition).Length() - radius > fogMaxDistance)
            {
                continue;
            }

            Vector3 extent = { radius, radius, radius };
            if (!frustum.IntersectsAABB(volume.position - extent, volume.position + extent))
            {
                continue;
            }

            volumetricFogPass_->SubmitFogVolume(volume);
        }
    }

    // バイラテラルブラーのパラメータ同期
    if (horizontalBilateralPass_ && verticalBilateralPass_)
    {
        auto* hSettings = horizontalBilateralPass_->GetSettings();
        auto* vSettings = verticalBilateralPass_->GetSettings();

        vSettings->depthTolerance = hSettings->depthTolerance;
        vSettings->normalTolerance = hSettings->normalTolerance;
    }
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // ゲームの更新でこのフレームに渡された演出も入れるため、描く直前に値を作る
    ComposeScreenEffects();

    // Contextセットアップとリソースバリア
    auto* offscreenRTV = engine_->GetOffscreenRTVManager();
    context_.srvManager = srvManager_;
    context_.rootSigManager = rootSignatureManager_;
    context_.sceneColorSrvIndex = sceneTextureIndex_;
    context_.sceneDepthSrvIndex = sceneDepthIndex_;
    context_.normalSrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<uint32_t>(GBufferIndex::Normal));

    // 共通ヒープの設定
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // エフェクト実行

    // SSAO & Bilateral Blur
    {
        ssaoPass_->Execute(cmdList, context_);

        // 横ブラー → 縦ブラー の順で適用
        horizontalBilateralPass_->Execute(cmdList, context_, ssaoPass_->GetSRVHandleGPU());
        verticalBilateralPass_->Execute(cmdList, context_, horizontalBilateralPass_->GetSRVHandleGPU());
    }

    // Bloom
    {
        brightPass_->Execute(cmdList, context_);
        bloomPass_->Execute(cmdList, context_, brightPass_->GetSRVHandleGPU());
    }

    // Depth of Field
    {
        dofPass_->Execute(cmdList, context_);
    }

    // Volumetric Fog
    {
        // 生のフォグを生成
        volumetricFogPass_->Execute(cmdList, context_);

        // フィルターパスに生フォグの情報を渡して実行
        volumetricFogBilateralPass_->SetRawFogSrvIndex(volumetricFogPass_->GetSRVIndex());
        volumetricFogBilateralPass_->Execute(cmdList, context_);
    }

    // 最終合成
    {
        cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("PostProcess"));

        auto GetCPUHandle = [&](uint32_t index) {
            return srvManager_->GetSRVHandleCPU_ForCopying(index);
            };

        compositePass_->SetupInputViews(
            engine_->GetGraphicsDevice()->GetDevice(),
            GetCPUHandle(sceneTextureIndex_),
            GetCPUHandle(bloomPass_->GetSRVIndex()),
            GetCPUHandle(dofPass_->GetSRVIndex()),
            GetCPUHandle(sceneDepthIndex_),
            GetCPUHandle(volumetricFogBilateralPass_->GetSRVIndex()),
            GetCPUHandle(verticalBilateralPass_->GetSRVIndex())
        );

        compositePass_->Execute(cmdList, context_);
        cmdList->SetDescriptorHeaps(1, heaps);
    }
}

void PostEffectManager::BeginFinalComposite(ID3D12GraphicsCommandList* cmdList)
{
    // SRV → RenderTarget (書き込み用)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        finalPassResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->OMSetRenderTargets(1, &finalPassRTVHandle_, FALSE, nullptr);
}

void PostEffectManager::EndFinalComposite(ID3D12GraphicsCommandList* cmdList)
{
    // RenderTarget → SRV (バックバッファ等の描画リソースとして使う用)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        finalPassResource_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);
}

void PostEffectManager::BindProperties(PropertyBinder& binder, const std::string& prefix)
{
    prefix_ = prefix;
    std::string p = prefix_.empty() ? "" : prefix_ + "/";

    if (!cbData_) return;

    // 前のシーンで重ねていた演出は持ち越さない
    ClearScreenEffects();

    // パス設定ポインタの取得
    BrightExtractSettings* brightSettings = GetBrightSettings();
    BloomSettings* bloomSettings = GetBloomSettings();
    FinalCompositeSettings* compositeSettings = GetCompositeSettings();
    DoFSettings* dofSettings = GetDoFSettings();
    SSAOSettings* ssaoSettings = GetSSAOSettings();
    BilateralBlurSettings* bilateralSettings = GetBilateralBlurSettings();
    VolumetricFogSettings* volFogSettings = GetVolumetricFogSettings();
    FogBilateralSettings* fogBilateralSettings = GetFogBilateralSettings();

    // 画面演出は ScreenEffectSettings に保存し、GPU に渡す値は ComposeScreenEffects で毎フレーム作る
    ScreenEffectSettings& screen = screenEffectSettings_;
    binder.Bind(p + "Vignette/Enable", &screen.isVignetteEnabled, false);
    binder.Bind(p + "Vignette/Amount", &screen.vignetteAmount, 0.0f, 0.1f, 0.0f, 10.0f);
    binder.Bind(p + "Vignette/Radius", &screen.vignetteRadius, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "Vignette/Softness", &screen.vignetteSoftness, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "Vignette/EllipseScale", &screen.vignetteEllipseScale, { 1.0f, 1.0f });
    binder.BindColor(p + "Vignette/Color", &screen.vignetteColor, screen.vignetteColor);

    binder.Bind(p + "ChromAberration/Enable", &screen.isChromaticAberrationEnabled, false);
    binder.Bind(p + "ChromAberration/Offset", &screen.chromaOffset, { 0.0f, 0.0f }, 0.001f);

    binder.Bind(p + "Pixelation/Enable", &screen.isPixelationEnabled, false);
    binder.Bind(p + "Pixelation/Size", &screen.pixelationSize, 1.0f, 0.5f, 1.0f, 64.0f);

    binder.Bind(p + "RadialBlur/Enable", &screen.isRadialBlurEnabled, false);
    binder.Bind(p + "RadialBlur/Strength", &screen.radialBlurStrength, 0.0f, 0.01f, 0.0f, 0.2f);
    binder.Bind(p + "RadialBlur/Center", &screen.radialBlurCenter, { 0.5f, 0.5f }, 0.01f, 0.0f, 1.0f);

    binder.Bind(p + "ScreenNoise/Enable", &screen.isScreenNoiseEnabled, false);
    binder.Bind(p + "ScreenNoise/Amount", &screen.noiseAmount, 0.0f, 0.01f, 0.0f, 1.0f);
    binder.Bind(p + "ScreenNoise/Speed", &screen.noiseSpeed, 1.0f, 0.1f, 0.0f, 10.0f);
    binder.Bind(p + "ScreenNoise/Scale", &screen.noiseScale, 0.5f, 0.01f, 0.0f, 1.0f);


    if (compositeSettings && ssaoSettings)
    {
        binder.BindBool(p + "SSAO/Enable", &compositeSettings->enableSSAO, false);
        binder.Bind(p + "SSAO/Radius", &ssaoSettings->radius, 1.0f, 0.1f, 0.1f, 50.0f);
        binder.Bind(p + "SSAO/Intensity", &ssaoSettings->intensity, 1.0f, 0.1f, 0.0f, 10.0f);
        binder.Bind(p + "SSAO/Bias", &ssaoSettings->bias, 0.005f, 0.001f, 0.0f, 1.0f);
        binder.Bind(p + "SSAO/SampleCount", &ssaoSettings->sampleCount, 16, 1.0f, 4, 64);
        binder.Bind(p + "SSAO/FadeStart", &ssaoSettings->fadeStart, 100.0f, 1.0f, 0.0f, 1000.0f);
        binder.Bind(p + "SSAO/FadeEnd", &ssaoSettings->fadeEnd, 200.0f, 1.0f, 0.0f, 1000.0f);

        if (bilateralSettings)
        {
            binder.Bind(p + "SSAO/BilateralDepthTolerance", &bilateralSettings->depthTolerance, 1.0f, 0.01f, 0.0f, 100.0f);
            binder.Bind(p + "SSAO/BilateralNormalTolerance", &bilateralSettings->normalTolerance, 32.0f, 0.5f, 0.0f, 256.0f);
        }
    }

    if (brightSettings)
    {
        binder.Bind(p + "Bloom/Threshold", &brightSettings->threshold, 1.0f, 0.1f, 0.0f, 10.0f);
        binder.Bind(p + "Bloom/ExtractIntensity", &brightSettings->intensity, 1.0f, 0.1f, 0.0f, 5.0f);
    }
    if (bloomSettings)
    {
        binder.Bind(p + "Bloom/Radius", &bloomSettings->radius, 1.0f, 0.05f, 0.0f, 5.0f);
    }
    if (compositeSettings)
    {
        binder.Bind(p + "Bloom/CompositeIntensity", &compositeSettings->bloomIntensity, 1.0f, 0.1f, 0.0f, 5.0f);
    }

    if (compositeSettings && dofSettings)
    {
        binder.BindBool(p + "DoF/Enable", &compositeSettings->enableDoF, false);
        binder.Bind(p + "DoF/FocusDistance", &dofSettings->focusDistance, 10.0f, 0.5f, 0.1f, 500.0f);
        binder.Bind(p + "DoF/FocusRange", &dofSettings->focusRange, 5.0f, 0.5f, 0.1f, 100.0f);
        binder.Bind(p + "DoF/BokehRadius", &dofSettings->bokehRadius, 5.0f, 0.5f, 1.0f, 50.0f);
        binder.Bind(p + "DoF/TransitionRange", &dofSettings->transitionRange, 10.0f, 0.5f, 0.1f, 100.0f);
        binder.Bind(p + "DoF/BokehHighlightThreshold", &dofSettings->bokehHighlightThreshold, 1.0f, 0.05f, 0.0f, 2.0f);
        binder.Bind(p + "DoF/BokehHighlightIntensity", &dofSettings->bokehHighlightIntensity, 1.0f, 1.0f, 0.0f, 200.0f);
    }

    if (compositeSettings && volFogSettings)
    {
        binder.BindBool(p + "VolumetricFog/Enable", &compositeSettings->enableVolumetricFog, false);
        binder.BindColor(p + "VolumetricFog/Albedo", &volFogSettings->albedo, { 0.8f, 0.8f, 0.8f });
        binder.Bind(p + "VolumetricFog/ExtinctionScale", &volFogSettings->extinctionScale, 0.2f, 0.01f, 0.0f, 10.0f);
        binder.Bind(p + "VolumetricFog/Anisotropy", &volFogSettings->anisotropy, 0.7f, 0.01f, -0.99f, 0.99f);
        binder.BindColor(p + "VolumetricFog/AmbientLight", &volFogSettings->ambientLight, { 0.0f, 0.0f, 0.0f });

        binder.Bind(p + "VolumetricFog/Extinction", &volFogSettings->extinction, 0.005f, 0.001f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/HeightDensity", &volFogSettings->heightDensity, 0.0f, 0.01f, 0.0f, 5.0f);
        binder.Bind(p + "VolumetricFog/BaseHeight", &volFogSettings->baseHeight, 0.0f, 0.5f, -100.0f, 100.0f);
        binder.Bind(p + "VolumetricFog/HeightFalloff", &volFogSettings->heightFalloff, 0.1f, 0.001f, 0.0f, 1.0f);

        binder.Bind(p + "VolumetricFog/NoiseScale", &volFogSettings->noiseScale, 0.01f, 0.001f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/NoiseDistortion", &volFogSettings->noiseDistortion, 0.15f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/WindSpeedMultiplier", &volFogSettings->windSpeedMultiplier, 1.0f, 0.05f, 0.0f, 10.0f);
        binder.Bind(p + "VolumetricFog/Coverage", &volFogSettings->coverage, 0.85f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/Erosion", &volFogSettings->erosion, 0.4f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/NoiseIntensity", &volFogSettings->noiseIntensity, 1.0f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/NoiseFeather", &volFogSettings->noiseFeather, 0.3f, 0.01f, 0.001f, 2.0f);

        binder.Bind(p + "VolumetricFog/MaxDistance", &volFogSettings->maxDistance, 500.0f, 5.0f, 10.0f, 5000.0f);
        binder.Bind(p + "VolumetricFog/TemporalWeight", &volFogSettings->temporalWeight, 0.05f, 0.01f, 0.01f, 0.5f);

        if (fogBilateralSettings)
        {
            binder.Bind(p + "VolumetricFog/BilateralBlurRadius", &fogBilateralSettings->blurRadius, 2, 1.0f, 1, 5);
            binder.Bind(p + "VolumetricFog/BilateralSpatialSigma", &fogBilateralSettings->spatialSigma, 2.0f, 0.1f, 0.1f, 10.0f);
            binder.Bind(p + "VolumetricFog/BilateralDepthSigma", &fogBilateralSettings->depthSigma, 0.001f, 0.0001f, 0.00001f, 0.1f);
        }

        BindFogVolumes(binder);
    }
}

// 配置式フォグの項目。ボリュームを足したり消したりしたら、要素の場所が変わるので登録し直す
void PostEffectManager::BindFogVolumes(PropertyBinder& binder)
{
    if (!volumetricFogPass_) return;

    const std::string p = prefix_.empty() ? "" : prefix_ + "/";
    auto& volumes = volumetricFogPass_->GetFogVolumesData();
    fogVolumeCount_ = static_cast<int>(volumes.size());

    binder.Bind(p + "FogVolumes/Count", &fogVolumeCount_, fogVolumeCount_, 1.0f, 0, static_cast<int>(MAX_FOG_VOLUMES));

    if (fogVolumeCount_ < 0) fogVolumeCount_ = 0;
    if (fogVolumeCount_ > static_cast<int>(MAX_FOG_VOLUMES)) fogVolumeCount_ = static_cast<int>(MAX_FOG_VOLUMES);

    volumes.resize(fogVolumeCount_);

    for (int i = 0; i < fogVolumeCount_; ++i)
    {
        std::string vp = p + "FogVolumes/" + std::to_string(i) + "/";
        auto& vol = volumes[i];

        binder.Bind(vp + "Type", &vol.type, vol.type, 1.0f, 0, 1);
        binder.Bind(vp + "IsVisible", &vol.isVisible, vol.isVisible);
        binder.Bind(vp + "Position", &vol.position, vol.position, 0.1f);
        binder.Bind(vp + "Rotation", &vol.rotation, vol.rotation, 1.0f);
        binder.Bind(vp + "Scale", &vol.scale, vol.scale, 0.1f, 0.1f, 1000.0f);
        binder.BindColor(vp + "Color", &vol.color, vol.color);
        binder.Bind(vp + "Density", &vol.density, vol.density, 0.01f, 0.0f, 10.0f);
        binder.Bind(vp + "BlendDistance", &vol.blendDistance, vol.blendDistance, 0.01f, 0.0f, 1.0f);
        binder.Bind(vp + "WindDirection", &vol.windDirection, vol.windDirection, 0.05f, -1.0f, 1.0f);
        binder.Bind(vp + "WindSpeed", &vol.windSpeed, vol.windSpeed, 0.01f, -5.0f, 5.0f);
        binder.Bind(vp + "NoiseScale", &vol.noiseScale, vol.noiseScale, 0.01f);
        binder.Bind(vp + "NoiseIntensity", &vol.noiseIntensity, vol.noiseIntensity, 0.01f, 0.0f, 1.0f);
        binder.Bind(vp + "Coverage", &vol.coverage, vol.coverage, 0.01f, 0.0f, 1.0f);
        binder.Bind(vp + "Erosion", &vol.erosion, vol.erosion, 0.01f, 0.0f, 1.0f);
        binder.Bind(vp + "NoiseFeather", &vol.noiseFeather, vol.noiseFeather, 0.01f, 0.001f, 2.0f);
        binder.Bind(vp + "DistortionAmount", &vol.distortionAmount, vol.distortionAmount, 0.01f, 0.0f, 1.0f);
        binder.Bind(vp + "HeightFalloff", &vol.heightFalloff, vol.heightFalloff, 0.05f, 0.0f, 10.0f);
    }
}

void PostEffectManager::DebugGui(PropertyBinder& binder)
{
    std::string p = prefix_.empty() ? "" : prefix_ + "/";

    if (ImGui::CollapsingHeader("ポストエフェクト設定"))
    {
        ImGui::Spacing();

        if (ImGui::TreeNode("画面演出"))
        {
            binder.Draw(p + "Vignette/Enable", "ビネット");
            if (screenEffectSettings_.isVignetteEnabled)
            {
                ImGui::Indent();
                binder.Draw(p + "Vignette/Amount", "強度");
                binder.Draw(p + "Vignette/Radius", "半径");
                binder.Draw(p + "Vignette/Softness", "ぼかし具合");
                binder.Draw(p + "Vignette/EllipseScale", "楕円スケール");
                binder.Draw(p + "Vignette/Color", "色");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            binder.Draw(p + "ChromAberration/Enable", "色収差");
            if (screenEffectSettings_.isChromaticAberrationEnabled)
            {
                ImGui::Indent();
                binder.Draw(p + "ChromAberration/Offset", "ズレ量 (X, Y)");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            binder.Draw(p + "Pixelation/Enable", "ドット化");
            if (screenEffectSettings_.isPixelationEnabled)
            {
                ImGui::Indent();
                binder.Draw(p + "Pixelation/Size", "ドットサイズ");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            binder.Draw(p + "RadialBlur/Enable", "ラディアルブラー");
            if (screenEffectSettings_.isRadialBlurEnabled)
            {
                ImGui::Indent();
                binder.Draw(p + "RadialBlur/Strength", "ブラー強度");
                binder.Draw(p + "RadialBlur/Center", "中心座標 (UV)");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            binder.Draw(p + "ScreenNoise/Enable", "スクリーンノイズ (砂嵐)");
            if (screenEffectSettings_.isScreenNoiseEnabled)
            {
                ImGui::Indent();
                binder.Draw(p + "ScreenNoise/Amount", "ノイズ量");
                binder.Draw(p + "ScreenNoise/Speed", "変化速度");
                binder.Draw(p + "ScreenNoise/Scale", "粒度スケール");
                ImGui::Unindent();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("SSAO"))
        {
            binder.Draw(p + "SSAO/Enable", "有効にする");
            binder.Draw(p + "SSAO/Radius", "サンプリング半径");
            binder.Draw(p + "SSAO/Intensity", "影の濃さ");
            binder.Draw(p + "SSAO/Bias", "バイアス");
            binder.Draw(p + "SSAO/SampleCount", "サンプル数");
            binder.Draw(p + "SSAO/FadeStart", "フェード開始距離");
            binder.Draw(p + "SSAO/FadeEnd", "フェード終了距離");

            ImGui::SeparatorText("ノイズ除去");
            binder.Draw(p + "SSAO/BilateralDepthTolerance", "深度の許容度");
            binder.Draw(p + "SSAO/BilateralNormalTolerance", "法線の許容度");

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ブルーム"))
        {
            binder.Draw(p + "Bloom/Threshold", "しきい値");
            binder.Draw(p + "Bloom/ExtractIntensity", "抽出強度");
            binder.Draw(p + "Bloom/Radius", "拡散半径");
            binder.Draw(p + "Bloom/CompositeIntensity", "合成強度");

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("被写界深度 (DoF)"))
        {
            binder.Draw(p + "DoF/Enable", "有効にする");
            binder.Draw(p + "DoF/FocusDistance", "ピント距離");
            binder.Draw(p + "DoF/FocusRange", "ピント範囲");
            binder.Draw(p + "DoF/BokehRadius", "ボケの強さ");
            binder.Draw(p + "DoF/TransitionRange", "ボケ移行距離");
            binder.Draw(p + "DoF/BokehHighlightThreshold", "玉ボケ閾値");
            binder.Draw(p + "DoF/BokehHighlightIntensity", "玉ボケ強度");

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ボリュメトリックフォグ"))
        {
            binder.Draw(p + "VolumetricFog/Enable", "有効にする");

            ImGui::SeparatorText("PBR 光学特性");
            binder.Draw(p + "VolumetricFog/Albedo", "散乱色");
            binder.Draw(p + "VolumetricFog/ExtinctionScale", "全体の濃さ倍率");
            binder.Draw(p + "VolumetricFog/Anisotropy", "前方散乱 (Anisotropy)");
            binder.Draw(p + "VolumetricFog/AmbientLight", "環境光");

            ImGui::SeparatorText("高さ・密度");
            binder.Draw(p + "VolumetricFog/Extinction", "一様な濃さ");
            binder.Draw(p + "VolumetricFog/HeightDensity", "高さフォグ最大密度");
            binder.Draw(p + "VolumetricFog/BaseHeight", "基準高さ");
            binder.Draw(p + "VolumetricFog/HeightFalloff", "高さ減衰率");

            ImGui::SeparatorText("ノイズ・形状");
            binder.Draw(p + "VolumetricFog/NoiseScale", "ノイズスケール");
            binder.Draw(p + "VolumetricFog/NoiseDistortion", "ノイズ歪み");
            binder.Draw(p + "VolumetricFog/WindSpeedMultiplier", "風速影響倍率");
            binder.Draw(p + "VolumetricFog/Coverage", "霧の量 (Coverage)");
            binder.Draw(p + "VolumetricFog/Erosion", "縁の削り");
            binder.Draw(p + "VolumetricFog/NoiseIntensity", "むらの強さ");
            binder.Draw(p + "VolumetricFog/NoiseFeather", "むらの境目のぼかし");

            ImGui::SeparatorText("描画・TAA");
            binder.Draw(p + "VolumetricFog/MaxDistance", "最大描画距離");
            binder.Draw(p + "VolumetricFog/TemporalWeight", "TAA蓄積ウェイト");

            ImGui::SeparatorText("ノイズ除去");
            binder.Draw(p + "VolumetricFog/BilateralBlurRadius", "ノイズ除去 半径");
            binder.Draw(p + "VolumetricFog/BilateralSpatialSigma", "ノイズ除去 Spatial Sigma");
            binder.Draw(p + "VolumetricFog/BilateralDepthSigma", "ノイズ除去 Depth Sigma");

            if (volumetricFogPass_)
            {
                auto& volumes = volumetricFogPass_->GetFogVolumesData();
                ImGui::Spacing();
                ImGui::SeparatorText("配置式フォグ");

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.15f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.70f, 0.22f, 1.00f));
                if (ImGui::Button("+ ボリュームを追加", ImVec2(-1, 0)) && volumes.size() < MAX_FOG_VOLUMES)
                {
                    volumes.push_back(VolumetricFogPass::FogVolumeData());
                    int newCount = static_cast<int>(volumes.size());

                    GlobalVariables::GetInstance()->SetValue(binder.GetGroupPath(), p + "FogVolumes/Count", newCount);
                    BindFogVolumes(binder);
                }
                ImGui::PopStyleColor(2);

                bool isErased = false;
                for (size_t i = 0; i < volumes.size(); ++i)
                {
                    std::string vp = p + "FogVolumes/" + std::to_string(i) + "/";
                    ImGui::PushID(static_cast<int>(i));

                    bool nodeOpen = ImGui::TreeNodeEx(("Volume " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);

                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 45.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.15f, 0.15f, 1.00f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.00f));
                    if (ImGui::Button("削除", ImVec2(50.0f, 0.0f)))
                    {
                        volumes.erase(volumes.begin() + i);
                        int newCount = static_cast<int>(volumes.size());

                        GlobalVariables::GetInstance()->SetValue(binder.GetGroupPath(), p + "FogVolumes/Count", newCount);
                        isErased = true;
                        ImGui::PopStyleColor(2);
                        if (nodeOpen) ImGui::TreePop();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::PopStyleColor(2);

                    if (nodeOpen)
                    {
                        binder.Draw(vp + "IsVisible", "デバッグ描画");
                        binder.Draw(vp + "Type", "タイプ (0:Sphere, 1:Box)");
                        binder.Draw(vp + "Position", "位置");
                        binder.Draw(vp + "Rotation", "回転");
                        binder.Draw(vp + "Scale", "サイズ/半径");
                        binder.Draw(vp + "Color", "散乱色");
                        binder.Draw(vp + "Density", "密度");
                        binder.Draw(vp + "BlendDistance", "境界ボカシ (Blend)");
                        binder.Draw(vp + "WindDirection", "風向き");
                        binder.Draw(vp + "WindSpeed", "風速");
                        binder.Draw(vp + "NoiseScale", "ノイズスケール");
                        binder.Draw(vp + "NoiseIntensity", "むらの強さ");
                        binder.Draw(vp + "Coverage", "霧の量 (Coverage)");
                        binder.Draw(vp + "Erosion", "縁の削り");
                        binder.Draw(vp + "NoiseFeather", "むらの境目のぼかし");
                        binder.Draw(vp + "DistortionAmount", "流体歪み強さ");
                        binder.Draw(vp + "HeightFalloff", "ローカル高さ減衰");

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }

                if (isErased)
                {
                    BindFogVolumes(binder);
                }
            }

            ImGui::TreePop();
        }
    }

    if (!volumetricFogPass_) return;

    const auto& volumes = volumetricFogPass_->GetFogVolumesData();
    for (const auto& vol : volumes)
    {
        if (!vol.isVisible) continue;

        Vector4 drawColor = { vol.color.x, vol.color.y, vol.color.z, 1.0f };
        if (drawColor.x == 0.0f && drawColor.y == 0.0f && drawColor.z == 0.0f)
        {
            drawColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }

        // 球体ボリューム
        if (vol.type == 0)
        {
            float radius = (std::max)(vol.scale.x, 0.1f);
            DebugDraw::DrawSphere(vol.position, radius, drawColor);
        }
        // ボックスボリューム
        else if (vol.type == 1)
        {
            Matrix4x4 rotMat = Matrix4x4::MakeRotateXYZ({
                Math::ToRadians(vol.rotation.x),
                Math::ToRadians(vol.rotation.y),
                Math::ToRadians(vol.rotation.z)
                });
            Vector3 fullSize = { vol.scale.x * 2.0f, vol.scale.y * 2.0f, vol.scale.z * 2.0f };
            DebugDraw::DrawOBB(vol.position, fullSize, rotMat, drawColor);
        }
    }
}


}