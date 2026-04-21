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
    rootSigManager_ = rootSigManager;

    // シーンカラー / 深度SRV
    sceneTextureIndex_ = engine->GetOffscreenRTVManager()->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Color));
    sceneDepthIndex_ = sceneDepthSrvIndex;

    // 輝度抽出
    brightPass_ = std::make_unique<BrightExtractPass>();
    brightPass_->Initialize(engine, width, height, psoManager);

    // 縮小サイズ
    UINT smallW = Math::MyMax(1u, width / 4);
    UINT smallH = Math::MyMax(1u, height / 4);

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

    // GodRay初期化
    UINT godRayW = Math::MyMax(1u, width / 2);
    UINT godRayH = Math::MyMax(1u, height / 2);
    godRayPass_ = std::make_unique<GodRayPass>();
    godRayPass_->Initialize(engine, godRayW, godRayH, psoManager);

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

    // ポストエフェクト定数バッファ
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    cbPostEffect_ = BufferManager::CreateBufferResource(device, sizeof(PostEffectData));
    cbPostEffect_->Map(0, nullptr, reinterpret_cast<void**>(&postEffectData_));

    // 最終出力用オフスクリーンRT（Create後にSRVIndexが更新される）
    auto [finalResource, finalRtvHandle, finalSrvIndex] =
        engine_->GetOffscreenRTVManager()->CreateOffscreenRenderTarget(
            width, height, Vector4(0, 0, 0, 1), DXGI_FORMAT_R16G16B16A16_FLOAT
        );

    finalPassResource_ = finalResource;
    finalPassRTVHandle_ = finalRtvHandle;
    finalPassSRVIndex_ = finalSrvIndex;

    // パラメータの初期値を設定
    postEffectData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    postEffectData_->pixelationSize = 2.386f;
    postEffectData_->screenResolution = Vector2(float(width), float(height));
    postEffectData_->grayscaleColorAmount = 1.0f;
    postEffectData_->sepiaColorAmount = 1.0f;
    postEffectData_->tintMulColorAmount = 1.0f;
    postEffectData_->tintAddColorAmount = 1.0f;
    postEffectData_->tintScreenColorAmount = 1.0f;
    postEffectData_->tintColor = Vector3(1.0f, 1.0f, 1.0f);
    postEffectData_->vignetteAmount = 0.294f;
    postEffectData_->vignetteRadius = 0.148f;
    postEffectData_->vignetteSoftness = 0.3f;
    postEffectData_->vignetteEllipseScale = Vector2(1.2f, 1.0f);
    postEffectData_->noiseAmount = 0.05f;
    postEffectData_->noiseSpeed = 1.0f;
    postEffectData_->noiseScale = 0.2f;
    postEffectData_->waveAmplitude = 0.01f;
    postEffectData_->waveFrequency = 15.0f;
    postEffectData_->waveDirection = 0;
    postEffectData_->waveSpeed = 2.0f;
    postEffectData_->fisheyeDistortion = 0.2f;
    postEffectData_->scanlineScrollSpeed = 0.2f;
    postEffectData_->scanlineColor = { 0.0f, 0.0f, 0.0f };
    postEffectData_->scanlineDirection = 0;
    postEffectData_->blockNoiseAmount = 0.5f;
    postEffectData_->blockNoiseSize = 16.0f;
    postEffectData_->noiseSpeed = 1.0f;
    postEffectData_->rgbSplitOffset = 0.003f;
    postEffectData_->filmGrainIntensity = 0.5f;
    postEffectData_->glitchBlockHeight = 0.5f;
    postEffectData_->glitchAmount = 0.1f;
    postEffectData_->glitchNoiseIntensity = 0.2f;
    postEffectData_->heatDistortionStrength = 0.02f;
    postEffectData_->heatNoiseScale = 20.0f;
    postEffectData_->heatSpeed = 5.0f;
    postEffectData_->vignetteColor = Vector3(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
    postEffectData_->turbulentStrength = 0.2f;
    postEffectData_->turbulentFrequency = 10.0f;
    postEffectData_->turbulentSpeed = 3.0f;
    postEffectData_->modeFlags[0] = 0;
    postEffectData_->modeFlags[1] = 0;
    postEffectData_->dissolveThreshold = 0.0f;     
    postEffectData_->dissolveEdgeWidth = 0.04f;    
    postEffectData_->dissolveEdgeIntensity = 4.0f; 
    postEffectData_->dissolveEdgeColor = Vector3(1.0f, 0.4f, 0.1f);
	postEffectData_->radialBlurCenter = Vector2(0.5f, 0.5f);
	postEffectData_->radialBlurStrength = 0.3f; 
}

void PostEffectManager::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition)
{
    // 時間依存エフェクト用
    if (postEffectData_)
    {
        postEffectData_->totalTime =
            static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    }

    // バイラテラルブラーのパラメータを縦・横で同期する処理
    if (horizontalBilateralPass_ && verticalBilateralPass_)
    {
        auto* hSettings = horizontalBilateralPass_->GetSettings();
        auto* vSettings = verticalBilateralPass_->GetSettings();

        // 許容度だけ同期
        vSettings->depthTolerance = hSettings->depthTolerance;
        vSettings->normalTolerance = hSettings->normalTolerance;
    }

    godRayPass_->Update(cameraPosition, viewMatrix, projectionMatrix, engine_->GetLightManager());
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // 1. Contextのセットアップ
    context_.srvManager = srvManager_;
    context_.rootSigManager = rootSigManager_;
    context_.sceneColorSrvIndex = sceneTextureIndex_;
    context_.sceneDepthSrvIndex = sceneDepthIndex_;
    context_.normalSrvIndex = engine_->GetOffscreenRTVManager()->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Normal));
    context_.materialSrvIndex = engine_->GetOffscreenRTVManager()->GetOffscreenSRVIndex(static_cast<UINT>(GBufferIndex::Material));

    // 深度をポストエフェクト用に読み取り状態へ
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);

    // SRVヒープとルートシグネチャ設定
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // ==========================================
    // 各種ポストエフェクトの実行
    // ==========================================

    // SSAOの実行
    // SSAO: デフォルトのシーン/深度/法線を使用
    ssaoPass_->Execute(cmdList, context_);

    // バイラテラルブラー: SSAOの出力を明示的に渡す
    horizontalBilateralPass_->Execute(cmdList, context_, ssaoPass_->GetSRVHandleGPU());
    verticalBilateralPass_->Execute(cmdList, context_, horizontalBilateralPass_->GetSRVHandleGPU());

    // SSR / GodRay: デフォルトのシーン画像を使用
    cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("PostProcess"));
    ssrPass_->Execute(cmdList, context_);
    godRayPass_->Execute(cmdList, context_);

    // Bloom生成: 
    // BrightPass はシーンから輝度抽出
    brightPass_->Execute(cmdList, context_);
    // Downsample は BrightPass の結果を縮小
    downsamplePass_->Execute(cmdList, context_, brightPass_->GetSRVHandleGPU());

    // Bloomのブラー連鎖: 前のパスの結果を次へ渡す
    D3D12_GPU_DESCRIPTOR_HANDLE bloomInput = downsamplePass_->GetSRVHandleGPU();
    for (int i = 0; i < 4; ++i)
    {
        verticalBlurPass_->Execute(cmdList, context_, bloomInput);
        horizontalBlurPass_->Execute(cmdList, context_, verticalBlurPass_->GetSRVHandleGPU());
        bloomInput = horizontalBlurPass_->GetSRVHandleGPU();
    }

    // DoF: デフォルトのシーン画像を使用
    bokehPass_->Execute(cmdList, context_);

    // ==========================================
    // 最終合成 (Combine)
    // ==========================================
    cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("PostProcess"));

    // 最終合成用のビューセットアップ
    combinePass_->SetupInputViews(
        engine_->GetGraphicsDevice()->GetDevice(),
        srvManager_->GetSRVHandleCPU_ForCopying(sceneTextureIndex_),
        srvManager_->GetSRVHandleCPU_ForCopying(horizontalBlurPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(bokehPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(sceneDepthIndex_),
        srvManager_->GetSRVHandleCPU_ForCopying(godRayPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(horizontalBilateralPass_->GetSRVIndex()), 
        srvManager_->GetSRVHandleCPU_ForCopying(ssrPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(TextureManager::GetInstance().Get(currentNoiseName_))
    );

    combinePass_->Execute(cmdList, context_);

    cmdList->SetDescriptorHeaps(1, heaps);

    // 深度を次フレーム用に書き込み状態へ戻す
    CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    cmdList->ResourceBarrier(1, &barrierBack);
}

void PostEffectManager::BeginFinalComposite(ID3D12GraphicsCommandList* cmdList)
{
    // SRVからRenderTargetへ遷移
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        finalPassResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    cmdList->ResourceBarrier(1, &barrier);

    // レンダーターゲットをセット
    cmdList->OMSetRenderTargets(1, &finalPassRTVHandle_, FALSE, nullptr);
}

void PostEffectManager::EndFinalComposite(ID3D12GraphicsCommandList* cmdList)
{
    // RenderTargetからSRVへ遷移 (バックバッファへ描画するため)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        finalPassResource_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);
}

}