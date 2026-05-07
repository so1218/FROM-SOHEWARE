#include "pch.h"
#include "BrightExtractPass.h"
#include "BufferManager.h"
#include "Engine.h"

namespace FE
{

void BrightExtractPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    InitializeBase(engine, w, h); 
    psoManager_ = pso;

    // 定数バッファ作成
    constantBuffer_ = BufferManager::CreateBufferResource(engine->GetGraphicsDevice()->GetDevice(), sizeof(BrightExtractSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // 初期値設定
    cbData_->threshold = 1.0f;
    cbData_->intensity = 0.80f;
}

void BrightExtractPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    D3D12_GPU_DESCRIPTOR_HANDLE sceneSrvGPU = context.GetGPUHandle(context.sceneColorSrvIndex);

    PreDraw(cmdList);

    ID3D12DescriptorHeap* heaps[] = { context.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootSignature(context.rootSigManager->GetRootSignature("PostProcess"));

    cmdList->SetPipelineState(psoManager_->GetPSO("BrightnessExtract"));
    cmdList->SetGraphicsRootDescriptorTable(2, sceneSrvGPU);
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress()); 

    // フルスクリーン描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}