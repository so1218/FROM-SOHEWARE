#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"

PostEffectManager::~PostEffectManager()
{
}

void PostEffectManager::Initialize(
    Engine* engine,
    UINT width,
    UINT height,
    RootSignatureManager* rootSigManager,
    PSOManager* psoManager,
    Camera* camera,
    SRVManager* srvManager,
    uint32_t sceneDepthSrvIndex)
{
    engine_ = engine;
    srvManager_ = srvManager;
    rootSigManager_ = rootSigManager;

    // シーンカラー / 深度SRV
    sceneTextureIndex_ = engine->offscreenRTVManager_->GetOffscreenSRVIndex();
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

    // DOF
    dofDownsamplePass_ = std::make_unique<DownsamplePass>();
    dofDownsamplePass_->Initialize(engine, smallW, smallH, psoManager);

    dofVerticalBlurPass_ = std::make_unique<BlurPass>();
    dofVerticalBlurPass_->Initialize(engine, smallW, smallH, psoManager, true);

    dofHorizontalBlurPass_ = std::make_unique<BlurPass>();
    dofHorizontalBlurPass_->Initialize(engine, smallW, smallH, psoManager, false);

    // 最終合成
    combinePass_ = std::make_unique<BloomCombinePass>();
    combinePass_->Initialize(engine, width, height, psoManager, srvManager);

    // ポストエフェクト定数バッファ
    ID3D12Device* device = engine->graphicsDevice_->GetDevice();
    cbPostEffect_ = BufferManager::CreateBufferResource(device, sizeof(PostEffectData));
    cbPostEffect_->Map(0, nullptr, reinterpret_cast<void**>(&postEffectData_));

    // 最終出力用オフスクリーンRT（Create後にSRVIndexが更新される）
    auto resultPair = engine_->offscreenRTVManager_->CreateOffscreenRenderTarget(
        width, height, Vector4(0, 0, 0, 1));

    finalPassResource_ = resultPair.first;
    finalPassRTVHandle_ = resultPair.second;
    finalPassSRVIndex_ = engine_->offscreenRTVManager_->GetOffscreenSRVIndex();

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
}

void PostEffectManager::Update()
{
    // 時間依存エフェクト用
    if (postEffectData_)
    {
        postEffectData_->totalTime =
            static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    }
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // 深度をポストエフェクト用に読み取り状態へ
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->offscreenDepthResource_.Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    cmdList->ResourceBarrier(1, &barrier);

    // SRVヒープとルートシグネチャ設定
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootSignature(
        rootSigManager_->GetRootSignature("PostProcess"));

    auto sceneSRV = srvManager_->GetSRVHandleGPU(sceneTextureIndex_);

    // Bloom生成
    brightPass_->Execute(cmdList, sceneSRV);
    downsamplePass_->Execute(cmdList, brightPass_->GetSRVHandleGPU());

    auto bloomInputSRV = downsamplePass_->GetSRVHandleGPU();
    for (int i = 0; i < 4; ++i)
    {
        verticalBlurPass_->Execute(cmdList, bloomInputSRV);
        horizontalBlurPass_->Execute(
            cmdList, verticalBlurPass_->GetSRVHandleGPU());
        bloomInputSRV = horizontalBlurPass_->GetSRVHandleGPU();
    }

    // DoF生成
    dofDownsamplePass_->Execute(cmdList, sceneSRV);
    auto dofInputSRV = dofDownsamplePass_->GetSRVHandleGPU();
    for (int i = 0; i < 3; ++i)
    {
        dofVerticalBlurPass_->Execute(cmdList, dofInputSRV);
        dofHorizontalBlurPass_->Execute(
            cmdList, dofVerticalBlurPass_->GetSRVHandleGPU());
        dofInputSRV = dofHorizontalBlurPass_->GetSRVHandleGPU();
    }

    // 最終合成
    combinePass_->SetupInputViews(
        engine_->graphicsDevice_->GetDevice(),
        srvManager_->GetSRVHandleCPU_ForCopying(sceneTextureIndex_),
        srvManager_->GetSRVHandleCPU_ForCopying(horizontalBlurPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(dofHorizontalBlurPass_->GetSRVIndex()),
        srvManager_->GetSRVHandleCPU_ForCopying(sceneDepthIndex_)
    );

    combinePass_->Execute(cmdList, sceneSRV);

    cmdList->SetDescriptorHeaps(1, heaps);

    // 深度を次フレーム用に書き込み状態へ戻す
    CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->offscreenDepthResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    cmdList->ResourceBarrier(1, &barrierBack);
}