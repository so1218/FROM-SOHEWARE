#include "pch.h"
#include "SSAOPass.h"
#include "Engine.h"
#include "RootSignatureManager.h"

namespace FE
{

void SSAOPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R8_UNORM);
    psoManager_ = psoManager;

    // 定数バッファ生成
    cbSSAO_ = BufferManager::CreateMappedConstantBuffer<SSAOSettings>(
        engine->GetGraphicsDevice()->GetDevice(),
        &ssaoData_
    );

    ssaoData_->radius = 1.0f;
    ssaoData_->intensity = 2.5f;
    ssaoData_->bias = 0.025f;      
    ssaoData_->sampleCount = 32;    

    ssaoData_->fadeStart = 50.0f;  
    ssaoData_->fadeEnd = 100.0f;
}

void SSAOPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(200, 50, 255), "SSAO Pass");

    D3D12_GPU_DESCRIPTOR_HANDLE normalSRV = context.GetGPUHandle(context.normalSrvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE depthSRV = context.GetGPUHandle(context.sceneDepthSrvIndex);

    PreDraw(cmdList);

    // SSAO用のルートシグネチャとPSOをセット
    cmdList->SetGraphicsRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("SSAO"));
    cmdList->SetPipelineState(psoManager_->GetPSO("SSAO"));

    cmdList->SetGraphicsRootConstantBufferView(0, cbSSAO_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(2, normalSRV);
    cmdList->SetGraphicsRootDescriptorTable(3, depthSRV);

    // 全画面ポリゴン描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}