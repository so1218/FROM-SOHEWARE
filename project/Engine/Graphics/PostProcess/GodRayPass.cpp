#include "pch.h"
#include "GodRayPass.h"
#include "BufferManager.h"
#include "LightManager.h"
#include "Engine.h"

namespace FE
{

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

void GodRayPass::Update(const Vector3& cameraPosition,
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix,
    LightManager* lightManager)
{
    // 光源位置のスクリーン座標変換
    Vector2 lightUV = { 0.5f, 0.5f };
    Vector3 lightColor = { 1.0f, 1.0f, 1.0f };

    if (lightManager)
    {
        auto dirLights = lightManager->GetDirectionalLightData();
        if (dirLights[0].enable)
        {
            Vector3 camPos = cameraPosition;
            Vector3 lightDir = dirLights[0].direction;
            Vector3 virtualPos = camPos + (lightDir * -5000.0f);

            Matrix4x4 matViewProj = viewMatrix * projectionMatrix;
            Vector4 clipPos = matViewProj.Transform({ virtualPos.x, virtualPos.y, virtualPos.z, 1.0f });

            if (clipPos.w > 0.0f)
            {
                Vector2 ndc = { clipPos.x / clipPos.w, clipPos.y / clipPos.w };
                lightUV.x = (ndc.x + 1.0f) * 0.5f;
                lightUV.y = (1.0f - ndc.y) * 0.5f;
            }

            lightColor = Vector3(
                dirLights[0].color.x * dirLights[0].intensity,
                dirLights[0].color.y * dirLights[0].intensity,
                dirLights[0].color.z * dirLights[0].intensity
            );
        }
    }

    // クラス内の定数バッファデータに直接書き込む
    cbData_->lightPosScreen = lightUV;
    cbData_->lightColor = lightColor;
}

void GodRayPass::Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
    D3D12_GPU_DESCRIPTOR_HANDLE overrideInput)
{
    D3D12_CPU_DESCRIPTOR_HANDLE sceneHandleCPU = context.GetCPUHandle(context.sceneColorSrvIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE depthHandleCPU = context.GetCPUHandle(context.sceneDepthSrvIndex);

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
    cmdList->SetGraphicsRootSignature(context.rootSigManager->GetRootSignature("PostProcess"));

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

}