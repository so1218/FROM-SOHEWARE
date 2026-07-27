#include "pch.h"
#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"
#include "SRVManager.h"

namespace FE
{

PostEffectManager::~PostEffectManager()
{
}

void PostEffectManager::Initialize(
    Engine* engine,
    UINT width,
    UINT height,
    RootSignatureManager* rootSigManager,
    PSOManager* psoManager,
    SRVManager* srvManager,
    uint32_t sceneDepthSrvIndex)
{
    engine_ = engine;
    srvManager_ = srvManager;
    rootSignatureManager_ = rootSigManager;

    // シーンカラー / 深度SRV
    sceneTextureIndex_ = engine->GetOffscreenRTVManager()->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Color));
    sceneDepthIndex_ = sceneDepthSrvIndex;

    // 輝度抽出
    brightPass_ = std::make_unique<BrightExtractPass>();
    brightPass_->Initialize(engine, width, height, psoManager);

    // 縮小サイズ
    UINT smallW = Math::MyMax(1u, width / 2);
    UINT smallH = Math::MyMax(1u, height / 2);

    // Bloom
    downsamplePass_ = std::make_unique<DownsamplePass>();
    downsamplePass_->Initialize(engine, smallW, smallH, psoManager);

    verticalBlurPass_ = std::make_unique<BlurPass>();
    verticalBlurPass_->Initialize(engine, smallW, smallH, psoManager, true);

    horizontalBlurPass_ = std::make_unique<BlurPass>();
    horizontalBlurPass_->Initialize(engine, smallW, smallH, psoManager, false);

    // DOF(Bokeh)の初期化
    UINT halfW = Math::MyMax(1u, width / 2);
    UINT halfH = Math::MyMax(1u, height / 2);

    bokehPass_ = std::make_unique<BokehBlurPass>();
    bokehPass_->Initialize(engine, halfW, halfH, psoManager);

    // 最終合成
    combinePass_ = std::make_unique<BloomCombinePass>();
    combinePass_->Initialize(engine, width, height, psoManager, srvManager);

    // SSAO初期化
    ssaoPass_ = std::make_unique<SSAOPass>();
    ssaoPass_->Initialize(engine, width, height, psoManager);

    // BilateralBlur初期化 (横)
    horizontalBilateralPass_ = std::make_unique<BilateralBlurPass>();
    horizontalBilateralPass_->Initialize(engine, width, height, psoManager);
    horizontalBilateralPass_->GetSettings()->direction = { 1.0f, 0.0f }; 

    // BilateralBlur初期化 (縦)
    verticalBilateralPass_ = std::make_unique<BilateralBlurPass>();
    verticalBilateralPass_->Initialize(engine, width, height, psoManager);
    verticalBilateralPass_->GetSettings()->direction = { 0.0f, 1.0f };

    // SSR初期化
    ssrPass_ = std::make_unique<SSRPass>();
    ssrPass_->Initialize(engine, width, height, psoManager);

    // VolumetricFog初期化
    UINT volFogW = Math::MyMax(1u, width / 2);
    UINT volFogH = Math::MyMax(1u, height / 2);
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
    cbData_->grayscaleColorAmount = 1.0f;
    cbData_->sepiaColorAmount = 1.0f;
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
    cbData_->waveAmplitude = 0.01f;
    cbData_->waveFrequency = 15.0f;
    cbData_->waveDirection = 0;
    cbData_->waveSpeed = 2.0f;
    cbData_->fisheyeDistortion = 0.2f;
    cbData_->scanlineScrollSpeed = 0.2f;
    cbData_->scanlineColor = { 0.0f, 0.0f, 0.0f };
    cbData_->scanlineDirection = 0;
    cbData_->blockNoiseAmount = 0.5f;
    cbData_->blockNoiseSize = 16.0f;
    cbData_->noiseSpeed = 1.0f;
    cbData_->rgbSplitOffset = 0.003f;
    cbData_->filmGrainIntensity = 0.5f;
    cbData_->glitchBlockHeight = 0.5f;
    cbData_->glitchAmount = 0.1f;
    cbData_->glitchNoiseIntensity = 0.2f;
    cbData_->heatDistortionStrength = 0.02f;
    cbData_->heatNoiseScale = 20.0f;
    cbData_->heatSpeed = 5.0f;
    cbData_->vignetteColor = Vector3(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
    cbData_->turbulentStrength = 0.2f;
    cbData_->turbulentFrequency = 10.0f;
    cbData_->turbulentSpeed = 3.0f;
    cbData_->flag[0] = 0;
    cbData_->flag[1] = 0;
    cbData_->dissolveThreshold = 0.0f;     
    cbData_->dissolveEdgeWidth = 0.04f;    
    cbData_->dissolveEdgeIntensity = 4.0f; 
    cbData_->dissolveEdgeColor = Vector3(1.0f, 0.4f, 0.1f);
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
    context_.normalSrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Normal));
    context_.materialSrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Material));
    context_.velocitySrvIndex = offscreenRTV->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Velocity));

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

    // SSR
 /*   {
        cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("PostProcess"));
        ssrPass_->Execute(cmdList, context_);
    }*/

    // Bloom
    {
        brightPass_->Execute(cmdList, context_);
        downsamplePass_->Execute(cmdList, context_, brightPass_->GetSRVHandleGPU());

        D3D12_GPU_DESCRIPTOR_HANDLE bloomInput = downsamplePass_->GetSRVHandleGPU();
        const int BLUR_ITERATIONS = 4; // 定数化してわかりやすく

        for (int i = 0; i < BLUR_ITERATIONS; ++i)
        {
            verticalBlurPass_->Execute(cmdList, context_, bloomInput);
            horizontalBlurPass_->Execute(cmdList, context_, verticalBlurPass_->GetSRVHandleGPU());
            bloomInput = horizontalBlurPass_->GetSRVHandleGPU();
        }
    }

    // Depth of Field
    {
        bokehPass_->Execute(cmdList, context_);
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

        combinePass_->SetupInputViews(
            engine_->GetGraphicsDevice()->GetDevice(),
            GetCPUHandle(sceneTextureIndex_),
            GetCPUHandle(horizontalBlurPass_->GetSRVIndex()),
            GetCPUHandle(bokehPass_->GetSRVIndex()),
            GetCPUHandle(sceneDepthIndex_),
            GetCPUHandle(volumetricFogBilateralPass_->GetSRVIndex()),
            GetCPUHandle(verticalBilateralPass_->GetSRVIndex()), 
            GetCPUHandle(ssrPass_->GetSRVIndex())
        );

        combinePass_->Execute(cmdList, context_);
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

}