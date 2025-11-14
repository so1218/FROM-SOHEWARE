#include "PostEffectManager.h"
#include "BufferManager.h"
#include "TimeManager.h"
#include "Engine.h"

#include <externals/DirectXTex/d3dx12.h>

PostEffectManager::~PostEffectManager()
{
    // Initialize で確保したものは、すべてここで解放する
    srvManager_->FreeSRV(brightExtractIndex_);
    srvManager_->FreeSRV(verticalBlurIndex_);
    srvManager_->FreeSRV(horizontalBlurIndex_);
    srvManager_->FreeSRV(bloomCombineIndex_);
    srvManager_->FreeSRV(neonIndex_);
    srvManager_->FreeSRV(depthExtractIndex_);
}

void PostEffectManager::Initialize(Engine* engine, ID3D12Device* device, OffscreenRTVManager* offscreenRTVManager, UINT width, UINT height,
    RootSignatureManager* rootSignatureManager, PSOManager* psoManager, Camera* camera, SRVManager* srvManager)
{
    engine_ = engine;
    offscreenRTVManager_ = offscreenRTVManager;
    rootSignatureManager_ = rootSignatureManager;
    psoManager_ = psoManager;
    srvManager_ = srvManager;
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 共通SRV設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // オフスクリーン描画ターゲットを生成する汎用関数
    auto createTarget = [this, &srvDesc](UINT w, UINT h) {
        const Vector4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // レンダーターゲットとRTVハンドルを作成
        auto [resource, rtvHandle] = offscreenRTVManager_->CreateOffscreenRenderTarget(w, h, clearColor);

        // SRVを登録
        uint32_t srvIndex = srvManager_->CreateSRV(resource.Get(), srvDesc);

        return std::make_tuple(resource, rtvHandle, srvIndex);
        };

    // シーンテクスチャ（入力元）のSRVインデックスを取得
    sceneTextureSRVIndex_ = engine_->offscreenRTVManager_->GetOffscreenSRVIndex();

    // 各ポストエフェクト用のターゲットを作成
    std::tie(brightExtractResource_, brightExtractRTVHandle_, brightExtractIndex_) = createTarget(width, height);
    std::tie(verticalBlurResource_, verticalBlurRTVHandle_, verticalBlurIndex_) = createTarget(width, height);
    std::tie(horizontalBlurResource_, horizontalBlurRTVHandle_, horizontalBlurIndex_) = createTarget(width, height);
    std::tie(bloomCombineResource_, bloomCombineRTVHandle_, bloomCombineIndex_) = createTarget(width, height);
    std::tie(neonResource_, neonRTVHandle_, neonIndex_) = createTarget(width, height);
    std::tie(depthExtractResource_, depthExtractRTVHandle_, depthExtractIndex_) = createTarget(width, height);

    // SRVヒープの作成（ポストエフェクト用）
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 3; // t0: scene, t1: blurred, t2: depth
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvTableHeap_));
    assert(SUCCEEDED(hr));

    // CPU/GPUハンドルの取得
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvTableHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvTableHeap_->GetGPUDescriptorHandleForHeapStart();

    UINT numToCopy = 1;

    // SRVのコピー（GPU可視ヒープへ）

    // t0 = シーンテクスチャ
    D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU = srvManager_->GetSRVHandleCPU_ForCopying(sceneTextureSRVIndex_);
    device->CopyDescriptors(
        1, &cpuHandle, &numToCopy,
        1, &sceneCPU, &numToCopy,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

    // t1 = ブラー済みテクスチャ
    cpuHandle.ptr += descriptorSize_;
    D3D12_CPU_DESCRIPTOR_HANDLE blurCPU = srvManager_->GetSRVHandleCPU_ForCopying(horizontalBlurIndex_);
    device->CopyDescriptors(
        1, &cpuHandle, &numToCopy,
        1, &blurCPU, &numToCopy,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

    // t2 = 深度テクスチャ
    cpuHandle.ptr += descriptorSize_;
    D3D12_CPU_DESCRIPTOR_HANDLE depthCPU = srvManager_->GetSRVHandleCPU_ForCopying(sceneDepthIndex_);
    device->CopyDescriptors(
        1, &cpuHandle, &numToCopy,
        1, &depthCPU, &numToCopy,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

    // SRVテーブル先頭のGPUハンドルを保存（描画時に使用）
    bloomCombineSRVTable_ = gpuHandle;

    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(PostEffectData));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&postEffectData_));
    
    // Bright Extract
    cbBrightExtract_ = BufferManager::CreateBufferResource(device, sizeof(BrightExtractSettings));
    cbBrightExtract_->Map(0, nullptr, reinterpret_cast<void**>(&brightExtractData_));

    // Blur（縦横共通）
    cbBlur_ = BufferManager::CreateBufferResource(device, sizeof(BlurSettings));
    cbBlur_->Map(0, nullptr, reinterpret_cast<void**>(&blurSettingsData_));

    // Bloom Combine 等
    cbBloom_ = BufferManager::CreateBufferResource(device, sizeof(CombineSetting));
    cbBloom_->Map(0, nullptr, reinterpret_cast<void**>(&combineSettingsData_));

    // Depth関連の定数バッファ作成
    cbDepthExtractVS_ = BufferManager::CreateBufferResource(device, sizeof(DepthExtractSettingsVS));
    cbDepthExtractVS_->Map(0, nullptr, reinterpret_cast<void**>(&depthExtractVSData_));

    cbDepthExtractPS_ = BufferManager::CreateBufferResource(device, sizeof(DepthExtractSettingsPS));
    cbDepthExtractPS_->Map(0, nullptr, reinterpret_cast<void**>(&depthExtractPSData_));

    brightExtractData_->threshold = 1.01f;
    brightExtractData_->intensity = 0.4f;

    blurSettingsData_->texelSize = { 0.004f, 0.004f };
    blurSettingsData_->blurStrength = 0.574f;

    combineSettingsData_->brightnessThreshold = 0.0f;
    combineSettingsData_->effectMode = 1;

    depthExtractVSData_->nearPlane = camera->GetNearClip();
    depthExtractVSData_->farPlane = camera->GetFarClip();
    depthExtractVSData_->invViewProjection = Matrix4x4::Inverse(camera->GetViewProjectionMatrix());

    depthExtractPSData_->nearPlane = camera->GetNearClip();
    depthExtractPSData_->farPlane = camera->GetFarClip();
    
    postEffectData_->mode = 0;
    postEffectData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
    postEffectData_->brightnessValue = 0.2f;
    postEffectData_->pixelationSize = 2.386f;
    postEffectData_->posterizationLevels = 4;
    postEffectData_->screenResolution = Vector2(float(width), float(height));
    postEffectData_->grayscaleColorAmount = 1.0f;
    postEffectData_->sepiaColorAmount = 1.0f;
    postEffectData_->invertColorAmount = 1.0f;
    postEffectData_->tintMulColorAmount = 1.0f;
    postEffectData_->tintAddColorAmount = 1.0f;
    postEffectData_->tintScreenColorAmount = 1.0f;
    postEffectData_->tintColor = Vector3(1.0f, 1.0f, 1.0f);
    postEffectData_->contrastValue = 1.0f;
    postEffectData_->saturationValue = 1.0f;
    postEffectData_->hueShiftAmount = 0.5f;
    postEffectData_->vignetteAmount = 0.294f;
    postEffectData_->vignetteRadius = 0.148f;
    postEffectData_->vignetteSoftness = 0.3f;
    postEffectData_->vignetteEllipseScale = Vector2(1.2f, 1.0f);
    postEffectData_->channelSwapMode = 0;
    postEffectData_->noiseAmount = 0.05f;
    postEffectData_->noiseSpeed = 1.0f;
    postEffectData_->noiseScale = 0.2f;
    postEffectData_->celShadingLevels = 4.0f;
    postEffectData_->normalOutlineThreshold = 0.1f;
    postEffectData_->normalOutlineThickness = 1.0f;
    postEffectData_->normalOutlineColor = Vector3(0.0f, 0.0f, 0.0f);
    postEffectData_->waveAmplitude = 0.01f;
    postEffectData_->waveFrequency = 15.0f;
    postEffectData_->waveDirection = 0;
    postEffectData_->waveSpeed = 2.0f;
    postEffectData_->fisheyeDistortion = 0.2f;
    postEffectData_->flashFrequency = 2.0f;
    postEffectData_->flashIntensity = 0.5f;
    postEffectData_->scanlineScrollSpeed = 0.2f; 
    postEffectData_->scanlineColor = { 0.0f, 0.0f, 0.0f }; 
    postEffectData_->scanlineDirection = 0;
    postEffectData_->blockNoiseAmount = 0.5f;
    postEffectData_->blockNoiseSize = 16.0f;
    postEffectData_->noiseSpeed = 1.0f;
    postEffectData_->solarizeThreshold = 0.5f;
    postEffectData_->multiPosterizeLevels = 4.0f;
    postEffectData_->rgbSplitOffset = 0.003f;
    postEffectData_->filmGrainIntensity = 0.5f;
    postEffectData_->glitchBlockHeight = 0.5f;
    postEffectData_->glitchAmount = 0.1f;
    postEffectData_->glitchNoiseIntensity = 0.2f;
    postEffectData_->edgeThreshold = 0.2f;
    postEffectData_->heatDistortionStrength = 0.02f;
    postEffectData_->heatNoiseScale = 20.0f;
    postEffectData_->heatSpeed = 5.0f;
    postEffectData_->vignetteColor = Vector3(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
    postEffectData_->shadowColor = Vector3(0.1f, 0.2f, 0.6f);
    postEffectData_->highlightColor = Vector3(1.0f, 0.85f, 0.6f);
    postEffectData_->splitToneStrength = 0.5f;
    postEffectData_->turbulentStrength = 0.2f;
    postEffectData_->turbulentFrequency = 10.0f;
    postEffectData_->turbulentSpeed = 3.0f;
    postEffectData_->roughEdgeThreshold = 0.5f; 
    postEffectData_->roughEdgeRoughness = 0.5f; 
    postEffectData_->roughEdgeNoiseScale = 10.0f; 
    postEffectData_->roughEdgeSpeed = 1.0f; 
    postEffectData_->roughEdgeColor = Vector3(1.0f, 1.0f, 1.0f); 
    postEffectData_->spiralBaseAmplitude = 5.0f;
    postEffectData_->spiralFrequency = 5.0f;
    postEffectData_->spiralDistanceFalloff = 1.0f;
    postEffectData_->spiralNoiseAmount = 0.5f;
    postEffectData_->spiralNoiseSpeed = 1.0f;
    postEffectData_->spiralNoiseScale = 5.0f;
    postEffectData_->spiralRotationSpeed = 1.0f;
    postEffectData_->spiralSpeed = 1.0f;
    postEffectData_->radialWaveSpeed = 0.02f;
    postEffectData_->radialWaveAmplitude = 20.0f;
    postEffectData_->radialWaveFrequency = 2.0f;
    postEffectData_->glowOutlineThreshold = 0.1f;
    postEffectData_->glowOutlineThickness = 1.0f;
    postEffectData_->glowOutlineColor = Vector3(1.0f, 0.8f, 0.2f);
    postEffectData_->glowOutlineIntensity = 2.0f;
    postEffectData_->modeFlags[0] = 0;
    postEffectData_->modeFlags[1] = 0;
    postEffectData_->fbmOctaves = 5;
    postEffectData_->fbmGain = 0.5f;
    postEffectData_->fbmLacunarity = 2.0f;
    postEffectData_->fbmSharpness = 0.1f;
    postEffectData_->fbmNoiseIntensity = 1.5f;
    postEffectData_->fbmNoiseColor = Vector3(0.2f, 0.8f, 1.0f);
    postEffectData_->flareColor = Vector3(1.0f, 0.85f, 0.6f);    
    postEffectData_->flareIntensity = 1.0f;
    postEffectData_->flareFalloff = 2.0f;
    postEffectData_->flareGhostDistance = 0.5f;
    postEffectData_->flareGhostIntensity = 0.4f;
    postEffectData_->flareStreakCount = 6.0f;
    postEffectData_->flareStreakSpeed = 1.0f;
    postEffectData_->flareStreakSharpness = 16.0f;
    postEffectData_->flareStreakIntensity = 0.8f;
    postEffectData_->ballRadiusValue = Vector2(0.25f, 0.25f);
    postEffectData_->ballPosition = Vector2(0.5f, 0.5f);  
    postEffectData_->ballNoiseAmount = 0.02f;             
    postEffectData_->ballTimeSpeed = 1.0f;
    postEffectData_->ballColorAdjustment = Vector3(1, 1, 1);
    postEffectData_->dotBlinkSize = 8.0f;
    postEffectData_->dotBlinkSpeed = 5.0f;
}

void PostEffectManager::Update()
{
    postEffectData_->totalTime = static_cast<float>(TimeManager::GetInstance()->GetTotalTime());
}

void PostEffectManager::SetMode(int mode)
{
    postEffectData_->mode = mode;
}

void PostEffectManager::ExecutePostEffects(ID3D12GraphicsCommandList* cmdList)
{
    // SRVヒープをセット
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvManager_->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // DepthStencilをSRVとして使用可能にする
    CD3DX12_RESOURCE_BARRIER barrierToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->depthStencilResource_.Get(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrierToSRV);

    // Depth Extract
    {
        // Depth Extract用にレンダーターゲットに遷移
        CD3DX12_RESOURCE_BARRIER barrierDepthExtract = CD3DX12_RESOURCE_BARRIER::Transition(
            depthExtractResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrierDepthExtract);

        // ルートシグネチャ・PSO・SRV・CBをセット
        cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetDepthExtractRootSignature());
        cmdList->SetPipelineState(psoManager_->psoDepth_.Get());
        cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sceneDepthIndex_));
        cmdList->SetGraphicsRootConstantBufferView(0, cbDepthExtractVS_->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, cbDepthExtractPS_->GetGPUVirtualAddress());

        // Depth Extract用のRTVをセットしてクリア
        cmdList->OMSetRenderTargets(1, &depthExtractRTVHandle_, FALSE, nullptr);
        float clearColor[4] = { 0, 0, 0, 1 };
        cmdList->ClearRenderTargetView(depthExtractRTVHandle_, clearColor, 0, nullptr);

        // フルスクリーン三角形で描画
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        // 描画後、SRVとして再利用可能に遷移
        CD3DX12_RESOURCE_BARRIER barrierDepthExtractToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            depthExtractResource_.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierDepthExtractToSRV);
    }

    // DepthStencilを元の書き込み状態に戻す
    CD3DX12_RESOURCE_BARRIER barrierToDepth = CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->depthStencilResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    cmdList->ResourceBarrier(1, &barrierToDepth);

    // 共通ルートシグネチャをセット
    cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetPostProcessRootSignature());

    // Bright Extract
    {
        CD3DX12_RESOURCE_BARRIER barrierBrightExtract = CD3DX12_RESOURCE_BARRIER::Transition(
            brightExtractResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrierBrightExtract);

        cmdList->SetPipelineState(psoManager_->psoExtract_.Get());
        cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(sceneTextureSRVIndex_));
        cmdList->SetGraphicsRootConstantBufferView(0, cbBrightExtract_->GetGPUVirtualAddress());

        cmdList->OMSetRenderTargets(1, &brightExtractRTVHandle_, FALSE, nullptr);
        float clearColor[4] = { 0, 0, 0, 1 };
        cmdList->ClearRenderTargetView(brightExtractRTVHandle_, clearColor, 0, nullptr);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        CD3DX12_RESOURCE_BARRIER barrierBrightExtractToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            brightExtractResource_.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierBrightExtractToSRV);
    }

    // Vertical Blur
    {
        CD3DX12_RESOURCE_BARRIER barrierVerticalBlur = CD3DX12_RESOURCE_BARRIER::Transition(
            verticalBlurResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrierVerticalBlur);

        cmdList->SetPipelineState(psoManager_->psoBlurY_.Get());
        cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(brightExtractIndex_));
        cmdList->SetGraphicsRootConstantBufferView(0, cbBlur_->GetGPUVirtualAddress());

        cmdList->OMSetRenderTargets(1, &verticalBlurRTVHandle_, FALSE, nullptr);
        float clearColor[4] = { 0, 0, 0, 1 };
        cmdList->ClearRenderTargetView(verticalBlurRTVHandle_, clearColor, 0, nullptr);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        CD3DX12_RESOURCE_BARRIER barrierVerticalBlurToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            verticalBlurResource_.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierVerticalBlurToSRV);
    }

    // Horizontal Blur
    {
        CD3DX12_RESOURCE_BARRIER barrierHorizontalBlur = CD3DX12_RESOURCE_BARRIER::Transition(
            horizontalBlurResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrierHorizontalBlur);

        cmdList->SetPipelineState(psoManager_->psoBlurX_.Get());
        cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(verticalBlurIndex_));
        cmdList->SetGraphicsRootConstantBufferView(0, cbBlur_->GetGPUVirtualAddress());

        cmdList->OMSetRenderTargets(1, &horizontalBlurRTVHandle_, FALSE, nullptr);
        float clearColor[4] = { 0, 0, 0, 1 };
        cmdList->ClearRenderTargetView(horizontalBlurRTVHandle_, clearColor, 0, nullptr);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        CD3DX12_RESOURCE_BARRIER barrierHorizontalBlurToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            horizontalBlurResource_.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierHorizontalBlurToSRV);
    }

    // Bloom Combine
    {
        CD3DX12_RESOURCE_BARRIER barrierBloomCombine = CD3DX12_RESOURCE_BARRIER::Transition(
            bloomCombineResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrierBloomCombine);

        cmdList->SetPipelineState(psoManager_->psoBloomCombine_.Get());
        ID3D12DescriptorHeap* heaps[] = { srvTableHeap_.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootDescriptorTable(1, bloomCombineSRVTable_);
        cmdList->SetGraphicsRootConstantBufferView(0, cbBloom_->GetGPUVirtualAddress());

        cmdList->OMSetRenderTargets(1, &bloomCombineRTVHandle_, FALSE, nullptr);
        float clearColor[4] = { 0, 0, 0, 1 };
        cmdList->ClearRenderTargetView(bloomCombineRTVHandle_, clearColor, 0, nullptr);

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        CD3DX12_RESOURCE_BARRIER barrierBloomCombineToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
            bloomCombineResource_.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrierBloomCombineToSRV);
    }
}

//void PostEffectManager::ExecuteNeonPostEffect(ID3D12GraphicsCommandList* cmdList)
//{
//    ID3D12DescriptorHeap* heaps[] = { offscreenRTVManager_->GetSRVDescriptorHeap() };
//    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
//
//    // BrightExtract → Blur Vertical → Blur Horizontal → Combine（省略可能）
//    // ここは ExecutePostEffects() の内容を参考に、neonIndex_ を使って処理
//
//    // 最後に合成（加算）
//    cmdList->SetPipelineState(psoManager_->pipelineStateAdditive_.Get());
//    cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignaturePostProcess_.Get());
//
//    // t0: 通常のシーン, t1: ネオンブラー済み
//    cmdList->SetGraphicsRootDescriptorTable(1, bloomCombineSRVTableNeon_);
//    cmdList->DrawInstanced(3, 1, 0, 0);
//}