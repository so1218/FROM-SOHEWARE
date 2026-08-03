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
    ShapeGenerator::SkydomeGenerator(vertices, indices, 64, 32);
    skydomeMesh_.Initialize(env.device->GetDevice(), vertices, indices);

    skydomeMaterialHandle_ = env.materialManager->CreateMaterial(env.device->GetDevice());
    skydomeMaterialHandle_.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (int i = 0; i < kFrameCount; ++i)
    {
        // WVPバッファの生成
        wvpResource_[i] = BufferManager::CreateMappedConstantBuffer(
            env.device->GetDevice(),
            &mappedWvp_[i]
        );

        // 天候バッファの生成
        AtmosphereSkyResource_[i] = BufferManager::CreateMappedConstantBuffer(
            env.device->GetDevice(),
            &mappedAtmosphereSky_[i]
        );

        // 天候の初期値
        mappedAtmosphereSky_[i]->cloudCoverage = { 0.35f, 0.7f };
        mappedAtmosphereSky_[i]->windVelocity = { 0.006f, 0.003f };
        mappedAtmosphereSky_[i]->cloudScale = 0.3f;
        mappedAtmosphereSky_[i]->cloudShadowDensity = 0.6f;
        mappedAtmosphereSky_[i]->skyGradientExponent = 0.6f;
        mappedAtmosphereSky_[i]->sunAtmosphereGlow = 0.5f;
        mappedAtmosphereSky_[i]->zenithColor = { 0.05f, 0.15f, 0.4f };
        mappedAtmosphereSky_[i]->horizonColor = { 0.4f, 0.6f, 0.8f };
        mappedAtmosphereSky_[i]->groundColor = { 0.2f, 0.2f, 0.2f };
        mappedAtmosphereSky_[i]->cloudBumpScale = 0.15f;        
        mappedAtmosphereSky_[i]->cloudEdgeSoftness = 0.15f;    
        mappedAtmosphereSky_[i]->cloudAbsorption = 0.7f;       
        mappedAtmosphereSky_[i]->cloudAmbientColor = { 0.08f, 0.12f, 0.2f };
    }
}

void SkydomeRenderer::BeginFrame()
{
    isSubmitted_ = false;
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void SkydomeRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t cloudNoiseSrvIndex, const AtmosphereSkyData& weather)
{
    currentTransform_ = worldTransform;
    currentColor_ = color;
    cloudTextureIndex_ = cloudNoiseSrvIndex;
    currentAtmosphereSkyData_ = weather;
    isSubmitted_ = true;
}

void SkydomeRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix)
{
    if (!isSubmitted_) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // WVP行列の更新（現在のフレーム用バッファへ）
    Matrix4x4 view = viewMatrix;
    view.m[3][0] = 0.0f; view.m[3][1] = 0.0f; view.m[3][2] = 0.0f; // 平行移動の除去
    Matrix4x4 wvpMatrix = currentTransform_.matWorld_ * view * projectionMatrix;
    memcpy(mappedWvp_[currentFrameIndex_], &wvpMatrix, sizeof(TransformationMatrix));

    // 天候データの更新（現在のフレーム用バッファへ）
    memcpy(mappedAtmosphereSky_[currentFrameIndex_], &currentAtmosphereSkyData_, sizeof(AtmosphereSkyData));

    // マテリアルカラーの更新
    skydomeMaterialHandle_.materialData->color = Math::Uint32ToColorVector(currentColor_);

    // 描画準備
    cmdList->SetPipelineState(env.psoManager->GetPSO("Skydome"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skydome"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &skydomeMesh_.GetVertexBufferView());
    cmdList->IASetIndexBuffer(&skydomeMesh_.GetIndexBufferView());

    // 各種定数バッファのバインド
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, wvpResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, skydomeMaterialHandle_.resource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, AtmosphereSkyResource_[currentFrameIndex_]->GetGPUVirtualAddress());

    // SRVのバインド
    cmdList->SetGraphicsRootDescriptorTable(5, env.srvManager->GetSRVHandleGPU(cloudTextureIndex_));

    // 描画
    cmdList->DrawIndexedInstanced(static_cast<UINT>(skydomeMesh_.GetIndexCount()), 1, 0, 0, 0);
}

}