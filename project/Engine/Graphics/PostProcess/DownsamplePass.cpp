#include "pch.h"
#include "DownsamplePass.h"
#include "BufferManager.h"
#include "Engine.h"
#include "SRVManager.h"

void DownsamplePass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // 親クラスで1/4サイズのRTV/SRVを作成
    InitializeBase(engine, w, h);

    psoManager_ = pso;
    engine_ = engine;

    // 定数バッファを作成
    constantBuffer_ = BufferManager::CreateBufferResource(
        engine->GetGraphicsDevice()->GetDevice(),
        sizeof(BlurSettings)
    );

    // 定数バッファをCPUから更新できるようにマップ
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    cbData_->texelSize = { 1.0f / Engine::GetClientWidth(), 1.0f / Engine::GetClientHeight()};
    cbData_->blurStrength = 0.0f;
}

void DownsamplePass::Execute(
    ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV
)
{
    // 描画前処理
    PreDraw(cmdList);

    // ダウンサンプル用PSOを設定
    cmdList->SetPipelineState(psoManager_->GetPSO("Downsample"));

    // 共通SRVヒープを設定
    ID3D12DescriptorHeap* heaps[] = {
        engine_->GetSRVManager()->GetSRVHeap()
    };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(
        0,
        constantBuffer_->GetGPUVirtualAddress()
    );
    cmdList->SetGraphicsRootDescriptorTable(2, inputSRV);

    // フルスクリーン三角形を描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 描画後処理
    PostDraw(cmdList);
}