#include "pch.h"
#include "SSRPass.h"
#include "Engine.h"

namespace FE
{

void SSRPass::Initialize(Engine* engine, UINT width, UINT height, PSOManager* psoManager)
{
    InitializeBase(engine, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    psoManager_ = psoManager;

    ID3D12Device* device = engine->GetGraphicsDevice()->GetDevice();
    auto* srvManager = engine->GetSRVManager();

    // 1. 定数バッファの作成
    constantBuffer_ = BufferManager::CreateMappedConstantBuffer<SSRSettings>(device, &cbData_);

    // CS化で超軽量になったため、ステップ数を増やして品質を向上させています
    cbData_->maxDistance = 50.0f;
    cbData_->stepSize = 0.05f; // より細かいステップで精細に
    cbData_->maxSteps = 64;    // (128にしても十分回る可能性があります)
    cbData_->thickness = 0.2f;

    // 2. DescriptorHeap の作成 (SRV4つ + UAV1つ を書き込むため余裕を持たせて作成)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&passHeap_));
    passHeap_->SetName(L"SSR_Heap");

    // 3. 出力先リソースの作成
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr,
        IID_PPV_ARGS(&outputRes_)
    );
    outputRes_->SetName(L"SSR_OutputRes");

    // 出力用UAVとSRVを作成
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    outputUavIndex_ = srvManager->Allocate();
    outputSrvIndex_ = srvManager->Allocate();

    device->CreateUnorderedAccessView(outputRes_.Get(), nullptr, &uavDesc, srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_));
    device->CreateShaderResourceView(outputRes_.Get(), &srvDesc, srvManager->GetSRVHandleCPU_ForCopying(outputSrvIndex_));
}

void SSRPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context, D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    PIXScopedEvent(cmdList, PIX_COLOR(50, 150, 255), "SSR Compute Pass");

    ID3D12Device* device = engine_->GetGraphicsDevice()->GetDevice();
    UINT handleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto* srvManager = engine_->GetSRVManager();

    // ==========================================
    // 1. バリア設定
    // ==========================================
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    // DepthバッファをCompute(SRV)で読めるように遷移
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    ));
    // 出力用テクスチャをUAVへ
    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        outputRes_.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    ));
    cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    // ==========================================
    // 2. パイプライン設定
    // ==========================================
    cmdList->SetComputeRootSignature(context.rootSigManager->GetRootSignature("SSR_CS"));
    cmdList->SetPipelineState(psoManager_->GetPSO("SSR_CS"));

    // ヒープ設定
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);
    D3D12_CPU_DESCRIPTOR_HANDLE destCPU = passHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE destGPU = passHeap_->GetGPUDescriptorHandleForHeapStart();

    // ==========================================
    // 3. ディスクリプタのコピー (テーブル構築)
    // ==========================================
    // t0, t1, t2, t3 の4つを連続してコピー
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 0, handleSize), context.GetCPUHandle(context.sceneColorSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 1, handleSize), context.GetCPUHandle(context.normalSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 2, handleSize), context.GetCPUHandle(context.sceneDepthSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 3, handleSize), context.GetCPUHandle(context.materialSrvIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // u0 のUAVをコピー
    device->CopyDescriptorsSimple(1, CD3DX12_CPU_DESCRIPTOR_HANDLE(destCPU, 4, handleSize), srvManager->GetSRVHandleCPU_ForCopying(outputUavIndex_), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ==========================================
    // 4. バインドと実行 (Dispatch)
    // ==========================================
    cmdList->SetComputeRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress()); // [0] b0: SSRSettings
    cmdList->SetComputeRootConstantBufferView(1, engine_->GetGlobalConstants()->GetResource()->GetGPUVirtualAddress()); // [1] b1: FrameData

    cmdList->SetComputeRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 0, handleSize)); // [2] SRV Table (t0~t3)
    cmdList->SetComputeRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(destGPU, 4, handleSize)); // [3] UAV Table (u0)

    UINT width = engine_->GetClientWidth();
    UINT height = engine_->GetClientHeight();

    // シェーダーの numthreads(8, 8, 1) に合わせてスレッドグループを計算
    cmdList->Dispatch((width + 7) / 8, (height + 7) / 8, 1);

    // ==========================================
    // 5. 終了処理とエクスポート
    // ==========================================
    std::vector<D3D12_RESOURCE_BARRIER> resetBarriers;
    // Depthを元の状態に戻す
    resetBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        engine_->GetOffscreenDepthResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    ));
    // SSR出力を次のパスでSRVとして読めるように遷移
    resetBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        outputRes_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    ));
    cmdList->ResourceBarrier(static_cast<UINT>(resetBarriers.size()), resetBarriers.data());

    // 次のポストエフェクトパスがこのSSR結果(テクスチャ)を参照できるようにセット
    this->textureResource_ = outputRes_;
    this->srvIndex_ = outputSrvIndex_;
}

}