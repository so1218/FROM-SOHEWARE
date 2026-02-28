#include "BlurPass.h"
#include "BufferManager.h"
#include "Engine.h"

void BlurPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, bool isVertical)
{
    InitializeBase(engine, w, h);
    engine_ = engine;
    psoManager_ = pso;
    isVertical_ = isVertical;

    constantBuffer_ = BufferManager::CreateBufferResource(engine->GetGraphicsDevice()->GetDevice(), sizeof(BlurSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // テクセルサイズ計算
    cbData_->texelSize = { 1.0f / w, 1.0f / h };
    cbData_->blurStrength = 2.0f;
}

void BlurPass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) 
{
    PreDraw(cmdList);

    // 縦横でPSOを切り替え
    cmdList->SetPipelineState(psoManager_->GetPSO(isVertical_ ? "BlurVertical" : "BlurHorizontal"));

    cmdList->SetGraphicsRootDescriptorTable(2, inputSRV);
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}