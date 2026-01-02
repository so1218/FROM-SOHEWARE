#include "BloomCombinePass.h"
#include "BufferManager.h"
#include "TimeManager.h"
#include "Engine.h"

void BloomCombinePass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, SRVManager* srvManager)
{
    InitializeBase(engine, w, h);
    psoManager_ = pso;
    srvManager_ = srvManager;

    ID3D12Device* device = engine->graphicsDevice_->GetDevice();

    // --- 1. CombineSettings (b0) 用のバッファ ---
    cb_ = BufferManager::CreateBufferResource(device, sizeof(CombineSettings));
    cb_->Map(0, nullptr, reinterpret_cast<void**>(&combineData_));

    combineData_->bloomIntensity = 0.8f;
    combineData_->focusDistance = 0.0f;
    combineData_->focusRange = 0.0f;

    combineData_->fogColor = Vector3(0.6f, 0.7f, 0.8f); 
    combineData_->fogStart = 10.0f; 
    combineData_->fogEnd = 50.0f;

    combineData_->enableDoF = false;
    combineData_->enableFog = false;

    // --- 2. 専用SRVヒープ作成 ---
    // Scene, Bloom, DoF, Depth の4枚分
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見える

    // passHeap_ というメンバ変数（ComPtr）に作成
    engine->graphicsDevice_->GetDevice()->CreateDescriptorHeap(
        &heapDesc, IID_PPV_ARGS(&passHeap_)
    );

    passHeap_->SetName(L"BloomCombinePass_Heap");

    // ディスクリプタのサイズ取得（SetupInputViewsで使用）
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void BloomCombinePass::SetupInputViews(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE sceneCPU, D3D12_CPU_DESCRIPTOR_HANDLE bloomCPU, D3D12_CPU_DESCRIPTOR_HANDLE dofCPU, D3D12_CPU_DESCRIPTOR_HANDLE depthCPU)
{
    // ★修正: SRVManagerのヒープではなく、自分専用のヒープの先頭を取得
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = passHeap_->GetCPUDescriptorHandleForHeapStart();

    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // バラバラの場所にあるSRVを、この専用ヒープに「連続して」コピーして集める

    // t0: Scene
    device->CopyDescriptorsSimple(1, destHandle, sceneCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += descriptorSize;

    // t1: Bloom
    device->CopyDescriptorsSimple(1, destHandle, bloomCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += descriptorSize;

    // t2: DoF
    device->CopyDescriptorsSimple(1, destHandle, dofCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    destHandle.ptr += descriptorSize;

    // t3: Depth
    device->CopyDescriptorsSimple(1, destHandle, depthCPU, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void BloomCombinePass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE /*unused*/)
{
    PreDraw(cmdList); // ビューポート設定など

    // 1. PSOの設定
    cmdList->SetPipelineState(psoManager_->GetPSO("BloomCombine"));

    // 2. RootSignatureの設定
    // ★重要: ここで使うルートシグネチャを指定します
    // RootSignatureManagerから取得するか、メンバ変数に持っているものを使います

    // 3. DescriptorHeapの設定
    // ★重要: SRVManagerの全体ヒープではなく、今回作った「専用ヒープ」だけをセットします
    ID3D12DescriptorHeap* heaps[] = { passHeap_.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 4. Root Parameterの設定
    // ※ここの番号(0, 1, 2)は、作成したRootSignatureの定義順と一致している必要があります！


    // [Parameter 1]: ConstantBuffer (b0: Settings)
    cmdList->SetGraphicsRootConstantBufferView(0, cb_->GetGPUVirtualAddress());

    // [Parameter 2]: ConstantBuffer (b1: FrameData)
    // カメラ情報など
    cmdList->SetGraphicsRootConstantBufferView(1, engine_->cameraManager_->GetCameraResource()->GetGPUVirtualAddress());

    // [Parameter 0]: DescriptorTable (t0 - t3) 
    // 専用ヒープの先頭(t0)を渡す
    cmdList->SetGraphicsRootDescriptorTable(2, passHeap_->GetGPUDescriptorHandleForHeapStart());

    // 描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}