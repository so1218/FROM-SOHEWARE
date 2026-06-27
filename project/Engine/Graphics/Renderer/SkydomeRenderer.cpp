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

    UINT wvpSize = (sizeof(TransformationMatrix) + 255) & ~255;
    UINT weatherSize = (sizeof(WeatherData) + 255) & ~255;

    for (int i = 0; i < kFrameCount; ++i)
    {
        // WVPバッファの生成
        wvpResource_[i] = BufferManager::CreateBufferResource(env.device->GetDevice(), wvpSize);
        wvpResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedWvp_[i]));

        // 天候バッファの生成
        weatherResource_[i] = BufferManager::CreateBufferResource(env.device->GetDevice(), weatherSize);
        weatherResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedWeather_[i]));

        // 天候の初期値を設定しておく
        mappedWeather_[i]->cloudCoverage = { 0.35f, 0.7f };
        mappedWeather_[i]->windVelocity = { 0.006f, 0.003f };
        mappedWeather_[i]->cloudScale = 0.3f;
        mappedWeather_[i]->cloudShadowDensity = 0.6f;
    }
}

void SkydomeRenderer::BeginFrame()
{
    isSubmitted_ = false;
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void SkydomeRenderer::Submit(const WorldTransform& worldTransform, uint32_t color, uint32_t skyCubeSrvIndex, uint32_t cloudNoiseSrvIndex, const WeatherData& weather)
{
    currentTransform_ = worldTransform;
    currentColor_ = color;
    skyTextureIndex_ = skyCubeSrvIndex;
    cloudTextureIndex_ = cloudNoiseSrvIndex;
    currentWeatherData_ = weather;
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
    memcpy(mappedWeather_[currentFrameIndex_], &currentWeatherData_, sizeof(WeatherData));

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
    cmdList->SetGraphicsRootConstantBufferView(4, weatherResource_[currentFrameIndex_]->GetGPUVirtualAddress());

    // SRVのバインド
    cmdList->SetGraphicsRootDescriptorTable(5, env.srvManager->GetSRVHandleGPU(skyTextureIndex_));
    cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(cloudTextureIndex_));

    // 描画
    cmdList->DrawIndexedInstanced(static_cast<UINT>(skydomeMesh_.GetIndexCount()), 1, 0, 0, 0);
}

}