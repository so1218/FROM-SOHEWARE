#include "DepthExtractPass.h"
#include "BufferManager.h"
#include "Engine.h"

void DepthExtractPass::Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso, RootSignatureManager* rootSig, Camera* camera)
{
    InitializeBase(engine, w, h); // 出力先作成
    psoManager_ = pso;
    rootSigManager_ = rootSig;

    // 定数バッファ作成
    cbVS_ = BufferManager::CreateBufferResource(engine->graphicsDevice_->GetDevice(), sizeof(DepthExtractSettingsVS));
    cbVS_->Map(0, nullptr, reinterpret_cast<void**>(&vsData_));

    cbPS_ = BufferManager::CreateBufferResource(engine->graphicsDevice_->GetDevice(), sizeof(DepthExtractSettingsPS));
    cbPS_->Map(0, nullptr, reinterpret_cast<void**>(&psData_));

    // カメラ情報の初期設定
    UpdateCamera(camera);
}

void DepthExtractPass::UpdateCamera(Camera* camera)
{
    if (!camera) return;
    vsData_->nearPlane = camera->GetNearClip();
    vsData_->farPlane = camera->GetFarClip();
    vsData_->invViewProjection = Matrix4x4::Inverse(camera->GetViewProjectionMatrix());

    psData_->nearPlane = camera->GetNearClip();
    psData_->farPlane = camera->GetFarClip();
}

// execute: inputSRVは「シーンの深度バッファ(DSV)のSRV」
void DepthExtractPass::Execute(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE depthSRV)
{
    PreDraw(cmdList);

    // ※DepthExtractは専用のRootSignatureを使っているためここでセット
    cmdList->SetGraphicsRootSignature(rootSigManager_->GetRootSignature("DepthExtract"));
    cmdList->SetPipelineState(psoManager_->GetPSO("Depth"));

    // ルートパラメータ設定 (DepthExtract.hlslに合わせて調整)
    // param 0: VS CB, param 1: PS CB, param 2: Depth Texture SRV と仮定
    // 元コード: RootParameter 0=VS_CB, 1=PS_CB, 2=SRV_Table
    cmdList->SetGraphicsRootConstantBufferView(0, cbVS_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, cbPS_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(2, depthSRV);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    PostDraw(cmdList);
}