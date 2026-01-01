//#include "PostEffectManager.h"
//#include "BufferManager.h"
//#include "TimeManager.h"
//#include "Engine.h"

#include "PostEffectManager.h"
#include "TimeManager.h"
#include "Engine.h"

PostEffectManager::~PostEffectManager()
{
    // 各Passのunique_ptrはここで自動破棄され、それに伴い各PassのSRVも自動解放されます
    // (depthPass_, brightPass_ などはここで勝手に消えます)

    // しかし、シーン深度のSRVインデックスだけは「単なる数値」として残っているため、
    // ここで手動で解放する必要があります。
    if (srvManager_ && sceneDepthIndex_ != 0)
    {
        srvManager_->FreeSRV(sceneDepthIndex_);
        sceneDepthIndex_ = 0;
    }
}

void PostEffectManager::Initialize(Engine* engine, UINT width, UINT height,
    RootSignatureManager* rootSigManager, PSOManager* psoManager,
    Camera* camera, SRVManager* srvManager)
{
    engine_ = engine;
    srvManager_ = srvManager;
    rootSigManager_ = rootSigManager;

    // シーンテクスチャ/深度のSRVインデックス取得
    sceneTextureIndex_ = engine->offscreenRTVManager_->GetOffscreenSRVIndex();
    // ※ depthStencilResource_ のSRVインデックスはSRVManagerで管理されている前提(sceneDepthIndex_)
    // もし管理されていないならここでCreateSRVするか、Engineから取得する必要があります
    sceneDepthIndex_ = 0; // ★仮定: Engine等から適切なIndexをもらってください

    // 1. 深度抽出パス (Full Size)
    depthPass_ = std::make_unique<DepthExtractPass>();
    depthPass_->Initialize(engine, width, height, psoManager, rootSigManager, camera);

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

    // 4. 合成パス (Full Size)
    combinePass_ = std::make_unique<BloomCombinePass>();
    combinePass_->Initialize(engine, width, height, psoManager, srvManager);

    ID3D12Device* device = engine->graphicsDevice_->GetDevice();
    cbPostEffect_ = BufferManager::CreateBufferResource(device, sizeof(PostEffectData));
    cbPostEffect_->Map(0, nullptr, reinterpret_cast<void**>(&postEffectData_));

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

    //if (verticalBlurPass_ && horizontalBlurPass_)
    //{
    //    // マスター（Horizontal / ImGuiで操作している方）
    //    auto* hSettings = horizontalBlurPass_->GetSettings();
    //    // スレーブ（Vertical / 自動で合わせる方）
    //    auto* vSettings = verticalBlurPass_->GetSettings();

    //    // --- 強さの同期 ---
    //    vSettings->blurStrength = hSettings->blurStrength;

    //    // --- テクセルサイズ（ぼかし幅）の同期 ---
    //    // ImGuiで texelSize.x をいじると、hSettings->texelSize.x が変わる。

    //    // HorizontalPass (横): X方向に値を持ち、Yは0にする
    //    hSettings->texelSize.y = 0.0f; // 横ブラーなので縦はずらさない

    //    // VerticalPass (縦): 横の設定(x)を、縦方向(y)に適用する
    //    vSettings->texelSize.x = 0.0f;               // 縦ブラーなので横はずらさない
    //    vSettings->texelSize.y = hSettings->texelSize.x; // 横のサイズを縦に適用
    //}
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // -------------------------------------------------------------
    // 1. Depth Extraction
    // -------------------------------------------------------------
    // DSV(深度バッファ)をSRVとして使うためのバリア
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            engine_->depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);

        // 深度バッファのSRVを使って描画 (sceneDepthIndex_は別途正しく設定されている前提)
        depthPass_->Execute(cmdList, srvManager_->GetSRVHandleGPU(sceneDepthIndex_));

        // 元に戻す
        auto barrierBack = CD3DX12_RESOURCE_BARRIER::Transition(
            engine_->depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList->ResourceBarrier(1, &barrierBack);
    }

    // -------------------------------------------------------------
    // 2. Post Process Chain (共通設定)
    // -------------------------------------------------------------
    // ここからは共通のSRVヒープを使う場合の設定
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("PostProcess"));

    // シーンテクスチャのGPUハンドル
    auto sceneSRV = srvManager_->GetSRVHandleGPU(sceneTextureIndex_);

    // --- A. Brightness Extraction ---
    // 入力: シーン画像 -> 出力: 高輝度部
    brightPass_->Execute(cmdList, sceneSRV);

    //downsamplePass_->Execute(cmdList, brightPass_->GetSRVHandleGPU());

    // --- B. Downsample & Vertical Blur ---
    // 入力: 高輝度部 -> 出力: 縦ブラー(縮小)
    // ※ダウンサンプル専用パスを作るのが丁寧ですが、今回はBlurPassで兼ねるか、
    //   BlurPassの前に「DownsamplePass」クラスを挟むと完璧です。
    //   ここでは簡単のため、VerticalBlurPassが縮小解像度を持っているのでそのまま突っ込みます。
    verticalBlurPass_->Execute(cmdList, brightPass_->GetSRVHandleGPU());

    // --- C. Horizontal Blur ---
    // 入力: 縦ブラー -> 出力: 横ブラー(完成したブルームテクスチャ)
    horizontalBlurPass_->Execute(cmdList, verticalBlurPass_->GetSRVHandleGPU());

    // -------------------------------------------------------------
    // 3. Bloom Combine
    // -------------------------------------------------------------
    // 合成パスは専用のヒープを使うため、必要なCPUハンドルを集める
    // コピー元のCPUハンドルを取得
    auto handleScene = srvManager_->GetSRVHandleCPU_ForCopying(sceneTextureIndex_);
    auto handleBlur = srvManager_->GetSRVHandleCPU_ForCopying(horizontalBlurPass_->GetSRVIndex());
    auto handleDepth = srvManager_->GetSRVHandleCPU_ForCopying(depthPass_->GetSRVIndex()); // リニア深度

    // 3つをCombinePass内のヒープにコピーしてセットアップ
    combinePass_->SetupInputViews(
        engine_->graphicsDevice_->GetDevice(),
        handleScene,
        handleBlur,
        handleDepth
    );

    // 実行 (内部でDescriptorHeapが切り替わる)
    combinePass_->Execute(cmdList, sceneSRV /*unused*/);

    // 必要ならメインのHeapに戻しておく
    cmdList->SetDescriptorHeaps(1, heaps);
}