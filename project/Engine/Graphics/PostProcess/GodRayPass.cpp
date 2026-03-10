#include "pch.h"
#include "GodRayPass.h"
#include "BufferManager.h"
#include "Engine.h"

void GodRayPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    InitializeBase(engine, w, h);
    psoManager_ = pso;

    // 設定用CB作成
    constantBuffer_ = BufferManager::CreateBufferResource(
        engine->GetGraphicsDevice()->GetDevice(), sizeof(GodRaySettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // デフォルトパラメータ設定
    cbData_->lightPosScreen = { 0.5f, 0.5f };
    cbData_->density = 0.075f;
    cbData_->decay = 0.96f; 
    cbData_->weight = 0.15f;  
    cbData_->exposure = 0.07f;
    cbData_->numSamples = 64; 
    cbData_->threshold = 0.8f;
    cbData_->sunRadius = 0.1f;

    // パス用SRVヒープ作成
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"GodRayPass_Heap");
}

void GodRayPass::Execute(ID3D12GraphicsCommandList* cmdList,
    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV)
{
    assert(false && "Use Execute(sceneSRV, depthSRV)");
}

void GodRayPass::Execute(ID3D12GraphicsCommandList* cmdList,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneHandleCPU,
    D3D12_CPU_DESCRIPTOR_HANDLE depthHandleCPU,
    const Vector2& lightPosUV)
{
    // パラメータ更新
    cbData_->lightPosScreen = lightPosUV;

    // ディスクリプタをパス用ヒープに集約コピー
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    device->CopyDescriptorsSimple(1, destHandle, sceneHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    destHandle.ptr += handleSize;
    device->CopyDescriptorsSimple(1, destHandle, depthHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 描画コマンド発行
    PreDraw(cmdList);

    cmdList->SetPipelineState(psoManager_->GetPSO("GodRay"));
    cmdList->SetGraphicsRootSignature(engine_->GetRootSignatureManager()->GetRootSignature("PostProcess"));

    // ヒープ設定
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // ルートパラメータ設定
    cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(2, passHeap_->GetGPUDescriptorHandleForHeapStart());

    // フルスクリーン描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}