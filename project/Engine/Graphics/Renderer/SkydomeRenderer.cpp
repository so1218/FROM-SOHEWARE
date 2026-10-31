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
#include "PIXColors.h"

namespace FS
{

void SkydomeRenderer::Initialize(const RenderEnvironment& env)
{
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    ShapeGenerator::SkydomeGenerator(vertices, indices, 64, 32);
    skydomeMesh_.Initialize(env.device->GetDevice(), vertices, indices);

    for (int frameIndex = 0; frameIndex < kFrameCount; ++frameIndex)
    {
        wvpResource_[frameIndex] = BufferManager::CreateMappedConstantBuffer(env.device->GetDevice(), &mappedWvp_[frameIndex]);
        atmosphereSkyResource_[frameIndex] = BufferManager::CreateMappedConstantBuffer(env.device->GetDevice(), &mappedAtmosphereSky_[frameIndex]);
    }
}

void SkydomeRenderer::BeginFrame()
{
    isSubmitted_ = false;
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void SkydomeRenderer::Submit(const WorldTransform& worldTransform, uint32_t cloudNoiseSrvIndex, const AtmosphereSkyData& atmosphereSkyData)
{
    currentTransform_ = worldTransform;
    cloudTextureIndex_ = cloudNoiseSrvIndex;
    currentAtmosphereSkyData_ = atmosphereSkyData;
    isSubmitted_ = true;
}

void SkydomeRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix)
{
    if (!isSubmitted_) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // スカイドーム描画パス全体のスコープ
    PIXScopedEvent(cmdList, FS::PIXColors::Geometry, "Skydome Pass");

    // カメラが動いても空が付いてくるよう、ビュー行列から平行移動を除く
    Matrix4x4 view = viewMatrix;
    view.m[3][0] = 0.0f;
    view.m[3][1] = 0.0f;
    view.m[3][2] = 0.0f;
    Matrix4x4 wvpMatrix = currentTransform_.matWorld_ * view * projectionMatrix;
    memcpy(mappedWvp_[currentFrameIndex_], &wvpMatrix, sizeof(TransformationMatrix));

    memcpy(mappedAtmosphereSky_[currentFrameIndex_], &currentAtmosphereSkyData_, sizeof(AtmosphereSkyData));

    cmdList->SetPipelineState(env.psoManager->GetPSO("Skydome"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skydome"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &skydomeMesh_.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&skydomeMesh_.GetIndexBufferView());

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, wvpResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, atmosphereSkyResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(cloudTextureIndex_));

    cmdList->DrawIndexedInstanced(static_cast<uint32_t>(skydomeMesh_.GetIndexCount()), 1, 0, 0, 0);
}

}