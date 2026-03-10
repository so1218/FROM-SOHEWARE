#include "pch.h"
#include "BokehBlurPass.h"
#include "BufferManager.h"
#include "Engine.h" 

void BokehBlurPass::Initialize(Engine* engine,
    UINT width,
    UINT height,
    PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);

    engine_ = engine;
    psoManager_ = psoManager;

    // DoF設定用定数バッファ
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(DoFSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    cbData_->focusDistance = 10.0f;
    cbData_->focusRange = 5.0f;
    cbData_->transitionRange = 5.0f;
    cbData_->bokehRadius = 5.0f;
    cbData_->bokehHighlightThreshold = 1.0f;
    cbData_->bokehHighlightIntensity = 50.0f;
}

void BokehBlurPass::Execute(ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV)
{
    assert(false && "Use Execute(sceneSRV, depthSRV)");
}

void BokehBlurPass::Execute(ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU,
    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvGPU)
{
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