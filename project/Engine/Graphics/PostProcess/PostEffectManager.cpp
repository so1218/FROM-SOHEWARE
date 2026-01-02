#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"

PostEffectManager::~PostEffectManager()
{
}

void PostEffectManager::Initialize(Engine* engine, UINT width, UINT height,
    RootSignatureManager* rootSigManager, PSOManager* psoManager,
    Camera* camera, SRVManager* srvManager,
    uint32_t sceneDepthSrvIndex)
{
    engine_ = engine;
    srvManager_ = srvManager;
    rootSigManager_ = rootSigManager;

    // シーンテクスチャ/深度のSRVインデックス取得
    sceneTextureIndex_ = engine->offscreenRTVManager_->GetOffscreenSRVIndex();
    // ※ depthStencilResource_ のSRVインデックスはSRVManagerで管理されている前提(sceneDepthIndex_)
    // もし管理されていないならここでCreateSRVするか、Engineから取得する必要があります
    sceneDepthIndex_ = sceneDepthSrvIndex; // ★仮定: Engine等から適切なIndexをもらってください

    // 2. 輝度抽出パス (Full Size)
    brightPass_ = std::make_unique<BrightExtractPass>();
    brightPass_->Initialize(engine, width, height, psoManager);

    // 3. ブラーパス (1/4 Size)
    UINT smallW = Math::MyMax(1u, width / 4);
    UINT smallH = Math::MyMax(1u, height / 4);

    downsamplePass_ = std::make_unique<DownsamplePass>();
    downsamplePass_->Initialize(engine, smallW, smallH, psoManager);

    verticalBlurPass_ = std::make_unique<BlurPass>();
    verticalBlurPass_->Initialize(engine, smallW, smallH, psoManager, true); // true = Vertical

    horizontalBlurPass_ = std::make_unique<BlurPass>();
    horizontalBlurPass_->Initialize(engine, smallW, smallH, psoManager, false); // false = Horizontal

    dofDownsamplePass_ = std::make_unique<DownsamplePass>();
    dofDownsamplePass_->Initialize(engine, smallW, smallH, psoManager);

    dofVerticalBlurPass_ = std::make_unique<BlurPass>();
    dofVerticalBlurPass_->Initialize(engine, smallW, smallH, psoManager, true); // true = Vertical

    dofHorizontalBlurPass_ = std::make_unique<BlurPass>();
    dofHorizontalBlurPass_->Initialize(engine, smallW, smallH, psoManager, false); // false = Horizontal

    // 4. 合成パス (Full Size)
    combinePass_ = std::make_unique<BloomCombinePass>();
    combinePass_->Initialize(engine, width, height, psoManager, srvManager);

    ID3D12Device* device = engine->graphicsDevice_->GetDevice();
    cbPostEffect_ = BufferManager::CreateBufferResource(device, sizeof(PostEffectData));
    cbPostEffect_->Map(0, nullptr, reinterpret_cast<void**>(&postEffectData_));

    // ここで新しいオフスクリーンRTを作ると、
      // OffscreenRTVManager内部の offscreenSrvIndex_ が「この新しいテクスチャ」のものに上書きされます。

    auto resultPair = engine_->offscreenRTVManager_->CreateOffscreenRenderTarget(
        width, height, Vector4(0.0f, 0.0f, 0.0f, 1.0f) // 黒クリア
    );

    // リソースとRTVハンドルを保存 (バリアやRTV設定で使用)
    finalPassResource_ = resultPair.first;
    finalPassRTVHandle_ = resultPair.second;

    // ★重要: 今作ったばかりのテクスチャのSRVインデックスを取得して保存
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
    // 必要ならカメラ情報の更新などをここで呼ぶ
    // depthPass_->UpdateCamera(camera); 
    if (postEffectData_)
    {
        postEffectData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
        // その他の動的パラメータ更新もここで行う
    }
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // -------------------------------------------------------------
    // 1. 深度バッファの準備 (バリア開始)
    // -------------------------------------------------------------
    // ★ここが最重要！
    // DepthPass(コピー)をしないなら、オリジナルの深度バッファを「読み取りモード」に変える必要があります。
    // そして、CombinePassが終わるまで「戻してはいけません」。
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->offscreenDepthResource_.Get(), // ※必ずオフスクリーンの深度リソースを指定
        D3D12_RESOURCE_STATE_DEPTH_WRITE,       // 書き込みモードから
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // 読み取りモードへ
    );
    cmdList->ResourceBarrier(1, &barrier);


    // -------------------------------------------------------------
    // 2. Post Process Chain (Bloom / DoF)
    // -------------------------------------------------------------
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("PostProcess"));

    auto sceneSRV = srvManager_->GetSRVHandleGPU(sceneTextureIndex_);

    // --- A. Bloom Generation ---
    brightPass_->Execute(cmdList, sceneSRV);
    downsamplePass_->Execute(cmdList, brightPass_->GetSRVHandleGPU());

    auto bloomInputSRV = downsamplePass_->GetSRVHandleGPU();
    const int bloomLoop = 4;
    for (int i = 0; i < bloomLoop; ++i) {
        verticalBlurPass_->Execute(cmdList, bloomInputSRV);
        horizontalBlurPass_->Execute(cmdList, verticalBlurPass_->GetSRVHandleGPU());
        bloomInputSRV = horizontalBlurPass_->GetSRVHandleGPU();
    }

    // --- B. DoF Generation ---
    dofDownsamplePass_->Execute(cmdList, sceneSRV);
    auto dofInputSRV = dofDownsamplePass_->GetSRVHandleGPU();
    const int dofLoop = 3;
    for (int i = 0; i < dofLoop; ++i) {
        dofVerticalBlurPass_->Execute(cmdList, dofInputSRV);
        dofHorizontalBlurPass_->Execute(cmdList, dofVerticalBlurPass_->GetSRVHandleGPU());
        dofInputSRV = dofHorizontalBlurPass_->GetSRVHandleGPU();
    }

    // -------------------------------------------------------------
    // 3. Bloom Combine (最終合成)
    // -------------------------------------------------------------
    // ここで初めて深度バッファ(t3)が読まれます。
    // さっき張ったバリアが効いているので、ここでは正常に読めるはずです。

    auto handleScene = srvManager_->GetSRVHandleCPU_ForCopying(sceneTextureIndex_);
    auto handleBloom = srvManager_->GetSRVHandleCPU_ForCopying(horizontalBlurPass_->GetSRVIndex());
    auto handleDoF = srvManager_->GetSRVHandleCPU_ForCopying(dofHorizontalBlurPass_->GetSRVIndex());
    auto handleDepth = srvManager_->GetSRVHandleCPU_ForCopying(sceneDepthIndex_); // 生の深度バッファSRV

    combinePass_->SetupInputViews(
        engine_->graphicsDevice_->GetDevice(),
        handleScene,
        handleBloom,
        handleDoF,
        handleDepth
    );

    combinePass_->Execute(cmdList, sceneSRV);

    cmdList->SetDescriptorHeaps(1, heaps);

    // -------------------------------------------------------------
    // 4. 後始末 (バリア終了)
    // -------------------------------------------------------------
    // ★読み終わったので、次回の描画のために「書き込みモード」に戻します。
    CD3DX12_RESOURCE_BARRIER barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->offscreenDepthResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    cmdList->ResourceBarrier(1, &barrierBack);
}