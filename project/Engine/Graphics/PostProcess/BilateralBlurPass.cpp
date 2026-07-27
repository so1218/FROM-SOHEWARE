#include "pch.h"
#include "BilateralBlurPass.h"
#include "Engine.h"
#include "RootSignatureManager.h"
#include "SRVManager.h"

namespace FE
{

BilateralBlurPass::~BilateralBlurPass()
{
    // 中間バッファ用のSRVを解放
    if (engine_->GetSRVManager() && intermediateSRVIndex_ != 0)
    {
        engine_->GetSRVManager()->FreeSRV(intermediateSRVIndex_);
        intermediateSRVIndex_ = 0;
    }
}

void BilateralBlurPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    // 縦ブラー結果（最終出力）用のバッファを初期化
    InitializeBase(engine, width, height, DXGI_FORMAT_R8_UNORM);
    psoManager_ = psoManager;

    // 中間バッファ（横ブラー結果用）の生成
    Vector4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);
    auto [resource, rtvHandle, srvIndex, uavIndex] = engine_->GetOffscreenRTVManager()->CreateOffscreenRenderTarget(
        width, height, clearColor, DXGI_FORMAT_R8_UNORM
    );
    intermediateResource_ = resource;
    intermediateRTV_ = rtvHandle;
    intermediateSRVIndex_ = srvIndex;

    // 定数バッファ生成
    auto device = engine->GetGraphicsDevice()->GetDevice();
   
    constantBufferBlurX_ = BufferManager::CreateMappedConstantBuffer<BilateralBlurSettings>(
        device,
        &cbDataBlurX_
    );

    constantBufferBlurY_ = BufferManager::CreateMappedConstantBuffer<BilateralBlurSettings>(
        device,
        &cbDataBlurY_
    );

    // パラメータの初期化
    settingsData_->texelSize = { 1.0f / width, 1.0f / height };
    settingsData_->depthTolerance = 1.0f;
    settingsData_->normalTolerance = 16.0f;

    *cbDataBlurX_ = *settingsData_;
    cbDataBlurX_->direction = { 1.0f, 0.0f }; // 横パス

    *cbDataBlurY_ = *settingsData_;
    cbDataBlurY_->direction = { 0.0f, 1.0f }; // 縦パス
}

void BilateralBlurPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(200, 50, 255), "Bilateral Blur Pass");

    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV = (overrideInput.ptr != 0)
        ? overrideInput
        : context.GetGPUHandle(context.sceneColorSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE depthSRV = context.GetGPUHandle(context.sceneDepthSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE normalSRV = context.GetGPUHandle(context.normalSrvIndex);

    // 定数バッファに反映
    cbDataBlurX_->depthTolerance = settingsData_->depthTolerance;
    cbDataBlurX_->normalTolerance = settingsData_->normalTolerance;
    cbDataBlurY_->depthTolerance = settingsData_->depthTolerance;
    cbDataBlurY_->normalTolerance = settingsData_->normalTolerance;

    // ルートシグネチャとPSOをセット
    cmdList->SetGraphicsRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("BilateralBlur"));
    cmdList->SetPipelineState(psoManager_->GetPSO("BilateralBlur"));
    cmdList->SetGraphicsRootConstantBufferView(1, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());

    // 横方向のブラー (SSAO -> 中間バッファ)

    // 中間バッファをSRVからRTVに変更
    auto barrierToRTV = CD3DX12_RESOURCE_BARRIER::Transition(
        intermediateResource_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &barrierToRTV);

    // 描画先を中間バッファに設定してクリア
    cmdList->RSSetViewports(1, &viewport_);
    cmdList->RSSetScissorRects(1, &scissorRect_);
    cmdList->OMSetRenderTargets(1, &intermediateRTV_, FALSE, nullptr);
    const float clearCol[] = { 0, 0, 0, 1 };
    cmdList->ClearRenderTargetView(intermediateRTV_, clearCol, 0, nullptr);

    cmdList->SetGraphicsRootConstantBufferView(0, constantBufferBlurX_->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(2, inputSRV);
    cmdList->SetGraphicsRootDescriptorTable(3, depthSRV); 
    cmdList->SetGraphicsRootDescriptorTable(4, normalSRV); 

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 中間バッファをRTVからSRVに
    auto barrierToSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        intermediateResource_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrierToSRV);

    // 縦方向のブラー (中間バッファ -> 最終バッファ)

    PreDraw(cmdList);

    cmdList->SetGraphicsRootConstantBufferView(0, constantBufferBlurY_->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE intermediateSRVHandle = engine_->GetSRVManager()->GetSRVHandleGPU(intermediateSRVIndex_);
    cmdList->SetGraphicsRootDescriptorTable(2, intermediateSRVHandle);

    cmdList->SetGraphicsRootDescriptorTable(3, depthSRV);
    cmdList->SetGraphicsRootDescriptorTable(4, normalSRV);

    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}