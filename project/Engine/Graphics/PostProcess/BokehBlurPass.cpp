#include "pch.h"
#include "BokehBlurPass.h"
#include "BufferManager.h"
#include "Engine.h" 

namespace FE
{

void BokehBlurPass::Initialize(Engine* engine,
    uint32_t width,
    uint32_t height,
    PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);

    engine_ = engine;
    psoManager_ = psoManager;

    // DoF設定用定数バッファ
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<DoFSettings>(
        engine->GetGraphicsDevice()->GetDevice(),
        &cbData_
    );

    cbData_->focusDistance = 10.0f;
    cbData_->focusRange = 5.0f;
    cbData_->transitionRange = 5.0f;
    cbData_->bokehRadius = 5.0f;
    cbData_->bokehHighlightThreshold = 1.0f;
    cbData_->bokehHighlightIntensity = 50.0f;
}

void BokehBlurPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(200, 50, 255), "Bokeh Blur Pass");

    D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU = context.GetGPUHandle(context.sceneColorSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGPU = context.GetGPUHandle(context.sceneDepthSrvIndex);

    // RTを描画状態へ
    PreDraw(cmdList);

    // PSO / RootSignature
    cmdList->SetGraphicsRootSignature(
        engine_->GetRootSignatureManager()->GetRootSignature("BokehBlur"));
    cmdList->SetPipelineState(
        psoManager_->GetPSO("BokehBlur"));

    // 定数バッファ
    cmdList->SetGraphicsRootConstantBufferView(
        0, constantBuffer_->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootConstantBufferView(
        1,
        engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());

    // SRV
    cmdList->SetGraphicsRootDescriptorTable(2, sceneSrvGPU);
    cmdList->SetGraphicsRootDescriptorTable(3, depthSrvGPU);

    cmdList->DrawInstanced(3, 1, 0, 0);

    // RTをSRVに戻す
    PostDraw(cmdList);
}

}