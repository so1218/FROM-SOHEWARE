#include "DownsamplePass.h"
#include "BufferManager.h"
#include "Engine.h"

void DownsamplePass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // 親クラスで 1/4 サイズのRTV/SRVを作成させる
    InitializeBase(engine, w, h);
    psoManager_ = pso;
    engine_ = engine;

    constantBuffer_ = BufferManager::CreateBufferResource(
        engine->graphicsDevice_->GetDevice(),
        sizeof(BlurSettings)
    );

    // ★追加: マッピング
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    cbData_->texelSize = { 1.0f / kClientWidth, 1.0f / kClientHeight };

    // その他はダミー
    cbData_->blurStrength = 0.0f;
}

void DownsamplePass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV)
{
    PreDraw(cmdList); // ビューポート設定、RTVセット、クリア

    // Downsample用のPSOを使用
    cmdList->SetPipelineState(psoManager_->GetPSO("Downsample"));

    // 共通ルートシグネチャ (t0に入力SRV)
    ID3D12DescriptorHeap* heaps[] = { engine_->srvManager_->GetSRVHeap() }; // IPostEffectが持つ自身のSRVヒープ(空の場合もある)
    // ※ 共通ヒープを使う場合は srvManager->GetSRVHeap() をセット

    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    // 入力テクスチャを t0 (ルートパラメータ 1番) にセット
    cmdList->SetGraphicsRootDescriptorTable(1, inputSRV);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList); // ResourceBarrierでSRVに戻す
}