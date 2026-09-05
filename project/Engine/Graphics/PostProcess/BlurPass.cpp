#include "pch.h"
#include "BlurPass.h"
#include "BufferManager.h"
#include "Engine.h"

namespace FE
{

void BlurPass::Initialize(Engine* engine, uint32_t w, uint32_t h, PSOManager* pso, bool isVertical)
{
    InitializeBase(engine, w, h);
    engine_ = engine;
    psoManager_ = pso;
    isVertical_ = isVertical;

    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<BlurSettings>(
        engine->GetGraphicsDevice()->GetDevice(),
        &cbData_
    );

    // テクセルサイズ計算
    cbData_->texelSize = { 1.0f / w, 1.0f / h };
    cbData_->blurStrength = 2.0f;
}

void BlurPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(200, 50, 255), "Blur Pass");

    // 入力が指定されていればそれを使う、なければデフォルトのSceneColorを使う
    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV = (overrideInput.ptr != 0)
        ? overrideInput
        : context.GetGPUHandle(context.sceneColorSrvIndex);

    PreDraw(cmdList);

    // 縦横でPSOを切り替え
    cmdList->SetPipelineState(psoManager_->GetPSO(isVertical_ ? "BlurVertical" : "BlurHorizontal"));

    cmdList->SetGraphicsRootDescriptorTable(2, inputSRV);
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}