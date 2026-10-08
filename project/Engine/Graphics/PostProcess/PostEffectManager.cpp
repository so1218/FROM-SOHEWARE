#include "pch.h"
#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"
#include "SRVManager.h"
#include "PropertyBinder.h"
#include "DebugDraw.h"

namespace FE
{

PostEffectManager::~PostEffectManager()
{
}

void PostEffectManager::Initialize(
    Engine* engine,
    uint32_t width,
    uint32_t height,
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

    // パラメータの初期値を設定
    cbData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    cbData_->pixelationSize = 2.386f;
    cbData_->screenResolution = Vector2(float(width), float(height));
    cbData_->tintMulColorAmount = 1.0f;
    cbData_->tintAddColorAmount = 1.0f;
    cbData_->tintScreenColorAmount = 1.0f;
    cbData_->tintColor = Vector3(1.0f, 1.0f, 1.0f);
    cbData_->vignetteAmount = 0.294f;
    cbData_->vignetteRadius = 0.148f;
    cbData_->vignetteSoftness = 0.3f;
    cbData_->vignetteEllipseScale = Vector2(1.2f, 1.0f);
    cbData_->noiseAmount = 0.05f;
    cbData_->noiseSpeed = 1.0f;
    cbData_->noiseScale = 0.2f;
    cbData_->chromaOffset = { 0.003f,0.0f };
    cbData_->vignetteColor = Vector3(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
    cbData_->flag = 0;
	cbData_->radialBlurCenter = Vector2(0.5f, 0.5f);
	cbData_->radialBlurStrength = 0.3f; 
}

void PostEffectManager::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition)
{
    // 時間依存データの更新
    if (cbData_)
    {
        cbData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    }

    if (cbData_)
    {
        cbData_->flag = 0;
        if (flagColorTint_)       cbData_->flag |= COLOR_TINT;
        if (flagVignette_)        cbData_->flag |= VIGNETTE;
        if (flagChromAberration_)  cbData_->flag |= CHROM_ABERRATION;
        if (flagPixelation_)      cbData_->flag |= PIXELATION;
        if (flagRadialBlur_)      cbData_->flag |= RADIAL_BLUR;
        if (flagScreenNoise_)     cbData_->flag |= SCREEN_NOISE;
        if (flagColorGradingLUT_)  cbData_->flag |= COLOR_GRADING_LUT;
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
    // Contextセットアップとリソースバリア
    auto* offscreenRTV = engine_->GetOffscreenRTVManager();
    context_.srvManager = srvManager_;
    context_.rootSigManager = rootSignatureManager_;
    context_.sceneColorSrvIndex = sceneTextureIndex_;
    context_.sceneDepthSrvIndex = sceneDepthIndex_;
    context_.normalSrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<uint32_t>(GBufferIndex::Normal));
    context_.materialSrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<uint32_t>(GBufferIndex::Material));

    // 深度バッファを読み取り用に遷移
    CD3DX12_RESOURCE_BARRIER depthToSrvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &depthToSrvBarrier);

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
        volumetricFogBilateralPass_->SetRawFogInput(
            volumetricFogPass_->GetResource(),   // リソースポインタ (バリア用)
            volumetricFogPass_->GetSRVIndex()    // SRVインデックス (コピー用)
        );
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

    // リソースバリアを戻す
    CD3DX12_RESOURCE_BARRIER depthToWriteBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    cmdList->ResourceBarrier(1, &depthToWriteBarrier);
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

    // パス設定ポインタの取得
    BrightExtractSettings* brightSettings = GetBrightSettings();
    BloomSettings* bloomSettings = GetBloomSettings();
    FinalCompositeSettings* compositeSettings = GetCompositeSettings();
    DoFSettings* dofSettings = GetDoFSettings();
    SSAOSettings* ssaoSettings = GetSSAOSettings();
    BilateralBlurSettings* bilateralSettings = GetBilateralBlurSettings();
    VolumetricFogSettings* volFogSettings = GetVolumetricFogSettings();
    FogBilateralSettings* fogBilateralSettings = GetFogBilateralSettings();

    flagColorTint_ = (cbData_->flag & COLOR_TINT) != 0;
    binder.Bind(p + "ColorTint/Enable", &flagColorTint_, flagColorTint_);
    binder.BindColor(p + "ColorTint/Color", &cbData_->tintColor, cbData_->tintColor);
    binder.Bind(p + "ColorTint/MulAmount", &cbData_->tintMulColorAmount, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "ColorTint/AddAmount", &cbData_->tintAddColorAmount, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "ColorTint/ScreenAmount", &cbData_->tintScreenColorAmount, 0.0f, 0.05f, 0.0f, 1.0f);

    flagVignette_ = (cbData_->flag & VIGNETTE) != 0;
    binder.Bind(p + "Vignette/Enable", &flagVignette_, flagVignette_);
    binder.Bind(p + "Vignette/Amount", &cbData_->vignetteAmount, 0.0f, 0.1f, 0.0f, 10.0f);
    binder.Bind(p + "Vignette/Radius", &cbData_->vignetteRadius, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "Vignette/Softness", &cbData_->vignetteSoftness, 0.0f, 0.05f, 0.0f, 1.0f);
    binder.Bind(p + "Vignette/EllipseScale", &cbData_->vignetteEllipseScale, { 1.0f, 1.0f });
    binder.BindColor(p + "Vignette/Color", &cbData_->vignetteColor, cbData_->vignetteColor);

    flagChromAberration_ = (cbData_->flag & CHROM_ABERRATION) != 0;
    binder.Bind(p + "ChromAberration/Enable", &flagChromAberration_, flagChromAberration_);
    binder.Bind(p + "ChromAberration/Offset", &cbData_->chromaOffset, { 0.0f, 0.0f });

    flagPixelation_ = (cbData_->flag & PIXELATION) != 0;
    binder.Bind(p + "Pixelation/Enable", &flagPixelation_, flagPixelation_);
    binder.Bind(p + "Pixelation/Size", &cbData_->pixelationSize, 1.0f, 0.5f, 1.0f, 64.0f);

    flagRadialBlur_ = (cbData_->flag & RADIAL_BLUR) != 0;
    binder.Bind(p + "RadialBlur/Enable", &flagRadialBlur_, flagRadialBlur_);
    binder.Bind(p + "RadialBlur/Strength", &cbData_->radialBlurStrength, 0.0f, 0.01f, 0.0f, 0.2f);
    binder.Bind(p + "RadialBlur/Center", &cbData_->radialBlurCenter, { 0.5f, 0.5f }, 0.01f, 0.0f, 1.0f);

    flagScreenNoise_ = (cbData_->flag & SCREEN_NOISE) != 0;
    binder.Bind(p + "ScreenNoise/Enable", &flagScreenNoise_, flagScreenNoise_);
    binder.Bind(p + "ScreenNoise/Amount", &cbData_->noiseAmount, 0.0f, 0.01f, 0.0f, 1.0f);
    binder.Bind(p + "ScreenNoise/Speed", &cbData_->noiseSpeed, 1.0f, 0.1f, 0.0f, 10.0f);
    binder.Bind(p + "ScreenNoise/Scale", &cbData_->noiseScale, 0.5f, 0.01f, 0.0f, 1.0f);

    flagColorGradingLUT_ = (cbData_->flag & COLOR_GRADING_LUT) != 0;
    binder.Bind(p + "LUT/Enable", &flagColorGradingLUT_, flagColorGradingLUT_);
    binder.BindTexture(p + "LUT/Texture", currentLutName_, [this](const std::string& name) {
        currentLutName_ = name;
        }, "LUT_Neutral_32", TextureType::LUT);

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
        binder.BindColor(p + "VolumetricFog/Albedo", &volFogSettings->albedo, volFogSettings->albedo);
        binder.Bind(p + "VolumetricFog/ScatteringIntensity", &volFogSettings->scatteringIntensity, 10.0f, 0.5f, 0.0f, 200.0f);
        binder.Bind(p + "VolumetricFog/ExtinctionScale", &volFogSettings->extinctionScale, 1.0f, 0.01f, 0.0f, 10.0f);
        binder.Bind(p + "VolumetricFog/Anisotropy", &volFogSettings->anisotropy, 0.5f, 0.01f, -0.99f, 0.99f);
        binder.BindColor(p + "VolumetricFog/AmbientLight", &volFogSettings->ambientLight, volFogSettings->ambientLight);

        binder.Bind(p + "VolumetricFog/Extinction", &volFogSettings->extinction, 0.01f, 0.001f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/HeightDensity", &volFogSettings->heightDensity, 0.5f, 0.01f, 0.0f, 5.0f);
        binder.Bind(p + "VolumetricFog/BaseHeight", &volFogSettings->baseHeight, 0.0f, 0.5f, -100.0f, 100.0f);
        binder.Bind(p + "VolumetricFog/HeightFalloff", &volFogSettings->heightFalloff, 0.1f, 0.001f, 0.0f, 1.0f);

        binder.Bind(p + "VolumetricFog/NoiseScale", &volFogSettings->noiseScale, 0.05f, 0.001f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/NoiseDistortion", &volFogSettings->noiseDistortion, 0.1f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/WindDirection", &volFogSettings->windDirection, { 1.0f, 0.0f, 0.0f }, 0.05f, -1.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/WindSpeed", &volFogSettings->windSpeed, 0.2f, 0.01f, -5.0f, 5.0f);
        binder.Bind(p + "VolumetricFog/Coverage", &volFogSettings->coverage, 0.5f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/WorleyWeight", &volFogSettings->worleyWeight, 0.5f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/Erosion", &volFogSettings->erosion, 0.2f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/ErosionStrength", &volFogSettings->erosionStrength, 0.5f, 0.01f, 0.0f, 2.0f);
        binder.Bind(p + "VolumetricFog/NoiseIntensity", &volFogSettings->noiseIntensity, 0.5f, 0.01f, 0.0f, 1.0f);
        binder.Bind(p + "VolumetricFog/NoiseFeather", &volFogSettings->noiseFeather, 0.3f, 0.01f, 0.001f, 2.0f);

        binder.Bind(p + "VolumetricFog/MaxDistance", &volFogSettings->maxDistance, 500.0f, 5.0f, 10.0f, 5000.0f);
        binder.Bind(p + "VolumetricFog/TemporalWeight", &volFogSettings->temporalWeight, 0.05f, 0.01f, 0.01f, 0.5f);

        if (fogBilateralSettings)
        {
            binder.Bind(p + "VolumetricFog/BilateralBlurRadius", &fogBilateralSettings->blurRadius, 2, 1.0f, 1, 5);
            binder.Bind(p + "VolumetricFog/BilateralSpatialSigma", &fogBilateralSettings->spatialSigma, 2.0f, 0.1f, 0.1f, 10.0f);
            binder.Bind(p + "VolumetricFog/BilateralDepthSigma", &fogBilateralSettings->depthSigma, 0.001f, 0.0001f, 0.00001f, 0.1f);
        }

        if (volumetricFogPass_)
        {
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
                binder.Bind(vp + "Anisotropy", &vol.anisotropy, vol.anisotropy, 0.01f, -0.99f, 0.99f);
                binder.Bind(vp + "WindDirection", &vol.windDirection, vol.windDirection, 0.05f, -1.0f, 1.0f);
                binder.Bind(vp + "WindSpeed", &vol.windSpeed, vol.windSpeed, 0.01f, -5.0f, 5.0f);
                binder.Bind(vp + "NoiseScale", &vol.noiseScale, vol.noiseScale, 0.01f);
                binder.Bind(vp + "NoiseIntensity", &vol.noiseIntensity, vol.noiseIntensity, 0.01f, 0.0f, 1.0f);
                binder.Bind(vp + "Coverage", &vol.coverage, vol.coverage, 0.01f, 0.0f, 1.0f);
                binder.Bind(vp + "WorleyWeight", &vol.worleyWeight, vol.worleyWeight, 0.01f, 0.0f, 1.0f);
                binder.Bind(vp + "Erosion", &vol.erosion, vol.erosion, 0.01f, 0.0f, 1.0f);
                binder.Bind(vp + "NoiseFeather", &vol.noiseFeather, vol.noiseFeather, 0.01f, 0.001f, 2.0f);
                binder.Bind(vp + "DistortionAmount", &vol.distortionAmount, vol.distortionAmount, 0.01f, 0.0f, 1.0f);
                binder.Bind(vp + "DensityOffset", &vol.densityOffset, vol.densityOffset, 0.01f, -1.0f, 1.0f);
                binder.Bind(vp + "NoiseContrast", &vol.noiseContrast, vol.noiseContrast, 0.05f, 0.0f, 10.0f);
                binder.Bind(vp + "HeightFalloff", &vol.heightFalloff, vol.heightFalloff, 0.05f, 0.0f, 10.0f);
            }
        }
    }
}

void PostEffectManager::DebugDraw(PropertyBinder& binder, const std::string& label)
{
#ifdef ENABLE_IMGUI
    std::string p = prefix_.empty() ? "" : prefix_ + "/";

    if (ImGui::TreeNode(label.c_str()))
    {
        if (ImGui::TreeNode("カラー・色調"))
        {
            binder.Draw(p + "ColorTint/Enable", "カラーティント");
            if (flagColorTint_)
            {
                ImGui::Indent();
                binder.Draw(p + "ColorTint/Color", "着色カラー");
                binder.Draw(p + "ColorTint/MulAmount", "乗算合成率");
                binder.Draw(p + "ColorTint/AddAmount", "加算合成率");
                binder.Draw(p + "ColorTint/ScreenAmount", "スクリーン合成率");
                ImGui::Unindent();
            }

            binder.Draw(p + "Vignette/Enable", "ビネット");
            if (flagVignette_)
            {
                ImGui::Indent();
                binder.Draw(p + "Vignette/Amount", "強度");
                binder.Draw(p + "Vignette/Radius", "半径");
                binder.Draw(p + "Vignette/Softness", "ぼかし具合");
                binder.Draw(p + "Vignette/EllipseScale", "楕円スケール");
                binder.Draw(p + "Vignette/Color", "色");
                ImGui::Unindent();
            }

            binder.Draw(p + "ChromAberration/Enable", "色収差");
            if (flagChromAberration_)
            {
                ImGui::Indent();
                binder.Draw(p + "ChromAberration/Offset", "ズレ量 (X, Y)");
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("形状・歪み"))
        {
            binder.Draw(p + "Pixelation/Enable", "ドット化");
            if (flagPixelation_)
            {
                ImGui::Indent();
                binder.Draw(p + "Pixelation/Size", "ドットサイズ");
                ImGui::Unindent();
            }

            binder.Draw(p + "RadialBlur/Enable", "ラディアルブラー");
            if (flagRadialBlur_)
            {
                ImGui::Indent();
                binder.Draw(p + "RadialBlur/Strength", "ブラー強度");
                binder.Draw(p + "RadialBlur/Center", "中心座標 (UV)");
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("特殊効果・ノイズ"))
        {
            binder.Draw(p + "ScreenNoise/Enable", "スクリーンノイズ (砂嵐)");
            if (flagScreenNoise_)
            {
                ImGui::Indent();
                binder.Draw(p + "ScreenNoise/Amount", "ノイズ量");
                binder.Draw(p + "ScreenNoise/Speed", "変化速度");
                binder.Draw(p + "ScreenNoise/Scale", "粒度スケール");
                ImGui::Unindent();
            }

            binder.Draw(p + "LUT/Enable", "カラーグレーディング (LUT)");
            if (flagColorGradingLUT_)
            {
                ImGui::Indent();
                binder.Draw(p + "LUT/Texture", "LUTテクスチャ");
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("SSAO"))
        {
            binder.Draw(p + "SSAO/Enable", "SSAO有効");
            binder.Draw(p + "SSAO/Radius", "サンプリング半径");
            binder.Draw(p + "SSAO/Intensity", "影の濃さ");
            binder.Draw(p + "SSAO/Bias", "バイアス");
            binder.Draw(p + "SSAO/SampleCount", "サンプル数");
            binder.Draw(p + "SSAO/FadeStart", "フェード開始距離");
            binder.Draw(p + "SSAO/FadeEnd", "フェード終了距離");
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
            binder.Draw(p + "DoF/Enable", "DoF有効");
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
            binder.Draw(p + "VolumetricFog/Albedo", "散乱色");
            binder.Draw(p + "VolumetricFog/ScatteringIntensity", "散乱の強さ");
            binder.Draw(p + "VolumetricFog/ExtinctionScale", "光の減衰スケール");
            binder.Draw(p + "VolumetricFog/Anisotropy", "前方散乱 (Anisotropy)");
            binder.Draw(p + "VolumetricFog/AmbientLight", "環境光");

            binder.Draw(p + "VolumetricFog/Extinction", "ベース密度");
            binder.Draw(p + "VolumetricFog/HeightDensity", "高さフォグ最大密度");
            binder.Draw(p + "VolumetricFog/BaseHeight", "基準高さ");
            binder.Draw(p + "VolumetricFog/HeightFalloff", "高さ減衰率");

            binder.Draw(p + "VolumetricFog/NoiseScale", "ノイズスケール");
            binder.Draw(p + "VolumetricFog/NoiseDistortion", "ノイズ歪み");
            binder.Draw(p + "VolumetricFog/WindDirection", "風向き");
            binder.Draw(p + "VolumetricFog/WindSpeed", "風速");
            binder.Draw(p + "VolumetricFog/Coverage", "霧の量 (Coverage)");
            binder.Draw(p + "VolumetricFog/WorleyWeight", "雲の塊感 (Worley)");
            binder.Draw(p + "VolumetricFog/Erosion", "削り取り強度");
            binder.Draw(p + "VolumetricFog/ErosionStrength", "流体融合削り強度");
            binder.Draw(p + "VolumetricFog/NoiseIntensity", "全体ノイズ適用度");
            binder.Draw(p + "VolumetricFog/NoiseFeather", "境界のボケ具合");

            binder.Draw(p + "VolumetricFog/MaxDistance", "最大描画距離");
            binder.Draw(p + "VolumetricFog/TemporalWeight", "TAA蓄積ウェイト");

            binder.Draw(p + "VolumetricFog/BilateralBlurRadius", "ノイズ除去 半径");
            binder.Draw(p + "VolumetricFog/BilateralSpatialSigma", "ノイズ除去 Spatial Sigma");
            binder.Draw(p + "VolumetricFog/BilateralDepthSigma", "ノイズ除去 Depth Sigma");

            // 配置式フォグ GUI
            if (volumetricFogPass_)
            {
                auto& volumes = volumetricFogPass_->GetFogVolumesData();
                ImGui::Separator();
                ImGui::Text("配置式フォグ (ボリューム数: %d)", static_cast<int>(volumes.size()));

                if (ImGui::Button("ボリュームを追加") && volumes.size() < MAX_FOG_VOLUMES)
                {
                    volumes.push_back(VolumetricFogPass::FogVolumeData());
                    BindProperties(binder, prefix_); // 追加時に再バインド
                }

                for (size_t i = 0; i < volumes.size(); ++i)
                {
                    std::string vp = p + "FogVolumes/" + std::to_string(i) + "/";
                    ImGui::PushID(static_cast<int>(i));

                    if (ImGui::TreeNode(("Volume " + std::to_string(i)).c_str()))
                    {
                        if (ImGui::Button("このボリュームを削除"))
                        {
                            volumes.erase(volumes.begin() + i);
                            BindProperties(binder, prefix_); // 削除時に再バインド
                            ImGui::TreePop();
                            ImGui::PopID();
                            break;
                        }

                        binder.Draw(vp + "IsVisible", "デバッグ描画");
                        binder.Draw(vp + "Type", "タイプ (0:Sphere, 1:Box)");
                        binder.Draw(vp + "Position", "位置");
                        binder.Draw(vp + "Rotation", "回転");
                        binder.Draw(vp + "Scale", "サイズ/半径");
                        binder.Draw(vp + "Color", "色");
                        binder.Draw(vp + "Density", "密度");
                        binder.Draw(vp + "BlendDistance", "境界ボカシ (Blend)");
                        binder.Draw(vp + "Anisotropy", "光の筋 (Anisotropy)");
                        binder.Draw(vp + "WindDirection", "風向き");
                        binder.Draw(vp + "WindSpeed", "風速");
                        binder.Draw(vp + "NoiseScale", "ノイズスケール");
                        binder.Draw(vp + "NoiseIntensity", "ノイズ強度");
                        binder.Draw(vp + "Coverage", "霧の量 (Coverage)");
                        binder.Draw(vp + "WorleyWeight", "雲の塊感");
                        binder.Draw(vp + "Erosion", "削り取り強度");
                        binder.Draw(vp + "NoiseFeather", "境界のボケ具合");
                        binder.Draw(vp + "DistortionAmount", "流体歪み強さ");
                        binder.Draw(vp + "DensityOffset", "密度の底上げ");
                        binder.Draw(vp + "NoiseContrast", "コントラスト");
                        binder.Draw(vp + "HeightFalloff", "ローカル高さ減衰");

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
#endif
}

}