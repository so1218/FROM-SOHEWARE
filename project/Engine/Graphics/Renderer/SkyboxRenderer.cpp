#include "pch.h"
#include "SkyboxRenderer.h"
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

void SkyboxRenderer::Initialize(const RenderEnvironment& env)
{
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;

    // ShapeGeneratorを使ってメッシュデータを生成
    ShapeGenerator::SkyBoxGenerator(vertices, indices);
    skyboxMesh_.Initialize(env.device->GetDevice(), vertices, indices);

    // WVP行列用のバッファを作成
    skyboxWvpResource_ = BufferManager::CreateBufferResource(env.device->GetDevice(), sizeof(TransformationMatrix));
    skyboxWvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedSkyboxWvp_));

    // マテリアルバッファを作成
    skyboxMaterialHandle_ = env.materialManager->CreateMaterial(env.device->GetDevice());
    skyboxMaterialHandle_.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void SkyboxRenderer::BeginFrame()
{
    isSubmitted_ = false;
}

void SkyboxRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
    currentTransform_ = worldTransform;
    currentColor_ = color;
    currentTextureIndex_ = cubeTextureSrvIndex;
    isSubmitted_ = true;
}

void SkyboxRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix)
{
    if (!isSubmitted_) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // WVP行列の計算（カメラの平行移動を除去して回転のみ反映）
    Matrix4x4 view = viewMatrix;
    view.m[3][0] = 0.0f;
    view.m[3][1] = 0.0f;
    view.m[3][2] = 0.0f;

    Matrix4x4 wvpMatrix = currentTransform_.matWorld_ * view * projectionMatrix;
    memcpy(mappedSkyboxWvp_, &wvpMatrix, sizeof(TransformationMatrix));

    // マテリアルカラー設定
    skyboxMaterialHandle_.materialData->color = Math::Uint32ToColorVector(currentColor_);

    // パイプライン設定
    cmdList->SetPipelineState(env.psoManager->GetPSO("Skybox"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skybox"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->IASetVertexBuffers(0, 1, &skyboxMesh_.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&skyboxMesh_.GetIndexBufferView());

    cmdList->SetGraphicsRootConstantBufferView(0, skyboxMaterialHandle_.resource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, skyboxWvpResource_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(currentTextureIndex_));

    cmdList->DrawIndexedInstanced(static_cast<uint32_t>(skyboxMesh_.GetIndexCount()), 1, 0, 0, 0);
}

}