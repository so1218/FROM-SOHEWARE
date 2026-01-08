#include "BrightExtractPass.h"
#include "BufferManager.h"
#include "Engine.h"

void BrightExtractPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    InitializeBase(engine, w, h); 
    psoManager_ = pso;

    // 定数バッファ作成
    constantBuffer_ = BufferManager::CreateBufferResource(engine->graphicsDevice_->GetDevice(), sizeof(BrightExtractSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // 初期値設定
    cbData_->threshold = 1.0f;
    cbData_->intensity = 0.80f;
}

void BrightExtractPass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV)
{
    PreDraw(cmdList);

    cmdList->SetPipelineState(psoManager_->GetPSO("BrightnessExtract"));
    cmdList->SetGraphicsRootDescriptorTable(2, inputSRV);
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress()); 

    // フルスクリーン描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}