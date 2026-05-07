#include "pch.h"
#include "VolumetricFogPass.h"
#include "Engine.h"

namespace FE
{

void VolumetricFogPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso)
{
    // ★重要：アルファチャンネル(透過率)も必要＆HDR値が入るのでFP16を指定
    InitializeBase(engine, w, h, DXGI_FORMAT_R16G16B16A16_FLOAT);
    psoManager_ = pso;

    // 設定用CB作成
    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(VolumetricFogSettings));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&cbData_));

    // デフォルトパラメータ設定 (好みに合わせて調整)
    cbData_->density = 0.05f;
    cbData_->scatteringG = 0.7f;   // 0より大きいと光源方向に強く散乱する(Mie散乱)
    cbData_->maxDistance = 150.0f; // 描画限界距離
    cbData_->steps = 32;           // 多いほど綺麗だが重い
    cbData_->intensity = 1.5f;

    cbData_->baseHeight = 0.0f;       // 地面(Y=0)を基準に
    cbData_->heightFalloff = 0.05f;   // 緩やかに上空で薄くする
    cbData_->noiseScale = 0.1f;       // ノイズのスケール
    cbData_->windSpeed = 1.0f;        // 風の強さ
    cbData_->noiseThreshold = 0.0f;
    cbData_->ambientFactor = 0.1f;
    cbData_->fogColor = { 1.0f, 1.0f, 1.0f };

    // パス用SRVヒープ作成（Depth, ShadowMapの 2つ分）
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2; // ★ここを2にする
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"VolumetricFog_Heap");
}

void VolumetricFogPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 1. CPUハンドルの取得
    // ① シーンの深度マップ (t0)
    D3D12_CPU_DESCRIPTOR_HANDLE depthHandleCPU = context.GetCPUHandle(context.sceneDepthSrvIndex);

    // ② シャドウマップ (t1)
    D3D12_CPU_DESCRIPTOR_HANDLE shadowHandleCPU = engine_->GetShadowMap()->GetSRVHandleCPU();

    // 2. ディスクリプタをパス用ヒープに集約コピー
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();

    device->CopyDescriptorsSimple(1, destHandle, depthHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += handleSize;
    device->CopyDescriptorsSimple(1, destHandle, shadowHandleCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 3. 描画開始処理
    PreDraw(cmdList);

    cmdList->SetPipelineState(psoManager_->GetPSO("VolumetricFog"));
    cmdList->SetGraphicsRootSignature(context.rootSigManager->GetRootSignature("VolumetricFog"));

    // ヒープ設定
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 4. ルートパラメータ設定
    // ※RootSignature生成時の AddCBV, AddDescriptorTableRange の順番に依存します。
    // 以下のインデックスは前回のRootSignatureの回答に基づいています。

    // Root Parameter 0: gFrameData (b0)
    // ★注意：お使いのエンジンの FrameData (Camera情報等) のGPUアドレスを渡してください。
    cmdList->SetGraphicsRootConstantBufferView(0, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress());

    // Root Parameter 1: gFogSettings (b2)
    cmdList->SetGraphicsRootConstantBufferView(1, constantBuffer_->GetGPUVirtualAddress());

    // Root Parameter 2: Texture Table (t0: Depth, t1: ShadowMap)
    cmdList->SetGraphicsRootDescriptorTable(2, passHeap_->GetGPUDescriptorHandleForHeapStart());

    // フルスクリーン描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}

}