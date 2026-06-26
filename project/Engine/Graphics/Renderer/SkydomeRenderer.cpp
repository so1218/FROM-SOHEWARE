#include "pch.h"
#include "SkydomeRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"
#include "ShapeGenerator.h"
#include "MaterialManager.h"

namespace FE
{

void SkydomeRenderer::Initialize(const RenderEnvironment& env)
{
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // 天球メッシュの生成 (分割数を増やすと空が滑らかになります)
    ShapeGenerator::SkydomeGenerator(vertices, indices, 64, 32);
    skydomeMesh_.Initialize(env.device->GetDevice(), vertices, indices);

    skydomeWvpResource_ = BufferManager::CreateBufferResource(env.device->GetDevice(), sizeof(TransformationMatrix));
    skydomeWvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedSkydomeWvp_));

    skydomeMaterialHandle_ = env.materialManager->CreateMaterial(env.device->GetDevice());
    skydomeMaterialHandle_.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void SkydomeRenderer::BeginFrame()
{
    isSubmitted_ = false;
}

void SkydomeRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t skyCubeSrvIndex, uint32_t cloudNoiseSrvIndex)
{
    currentTransform_ = worldTransform;
    currentColor_ = color;
    skyTextureIndex_ = skyCubeSrvIndex;
    cloudTextureIndex_ = cloudNoiseSrvIndex;
    isSubmitted_ = true;
}

void SkydomeRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix)
{
    if (!isSubmitted_) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // カメラの平行移動を除去（空が常にカメラの中心に来るようにするトリック）
    Matrix4x4 view = viewMatrix;
    view.m[3][0] = 0.0f;
    view.m[3][1] = 0.0f;
    view.m[3][2] = 0.0f;

    Matrix4x4 wvpMatrix = currentTransform_.matWorld_ * view * projectionMatrix;
    memcpy(mappedSkydomeWvp_, &wvpMatrix, sizeof(TransformationMatrix));

    skydomeMaterialHandle_.materialData->color = Math::Uint32ToColorVector(currentColor_);

    // PSOとRootSignatureのセット
    cmdList->SetPipelineState(env.psoManager->GetPSO("Skydome"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skydome"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->IASetVertexBuffers(0, 1, &skydomeMesh_.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&skydomeMesh_.GetIndexBufferView());

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootConstantBufferView(1, skydomeWvpResource_->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootConstantBufferView(3, skydomeMaterialHandle_.resource->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(skyTextureIndex_));

    cmdList->SetGraphicsRootDescriptorTable(5, env.srvManager->GetSRVHandleGPU(cloudTextureIndex_));

    // 描画
    cmdList->DrawIndexedInstanced(static_cast<UINT>(skydomeMesh_.GetIndexCount()), 1, 0, 0, 0);
}

}