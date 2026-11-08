#include "pch.h"
#include "TerrainRenderer.h"
#include "ShadowMap.h"
#include "BufferManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "CommandManager.h"
#include "GraphicsDevice.h"
#include "PIXColors.h"

namespace FS
{

void TerrainRenderer::Initialize(const RenderEnvironment& env)
{
    settingsBuffer_ = BufferManager::CreateMappedConstantBuffer(env.device->GetDevice(), &mappedSettings_);
    *mappedSettings_ = {};

    chunkBuffer_ = BufferManager::CreateMappedBuffer(env.device->GetDevice(), kMaxChunkCount, &mappedChunks_);
}

void TerrainRenderer::BeginFrame()
{
    isSubmitted_ = false;
    chunkCount_ = 0;
}

void TerrainRenderer::Submit(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks)
{
    drawData_ = drawData;
    isSubmitted_ = true;

    // GPU が描き終わるのを毎フレーム待っているので、ここで直接書き換え
    *mappedSettings_ = drawData.settings;

    chunkCount_ = static_cast<uint32_t>(std::min<size_t>(visibleChunks.size(), kMaxChunkCount));
    if (chunkCount_ > 0)
    {
        std::memcpy(mappedChunks_, visibleChunks.data(), sizeof(TerrainChunkData) * chunkCount_);
    }
}

void TerrainRenderer::Draw(const RenderEnvironment& env, ShadowMap* shadowMap)
{
    if (!isSubmitted_ || chunkCount_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    PIXScopedEvent(cmdList, FS::PIXColors::Geometry, "Terrain Pass (Chunks: %u)", chunkCount_);

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Terrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("Terrain"));

    const TerrainMaterial& material = *drawData_.material;
    auto SetTexture = [&](uint32_t rootIndex, uint32_t srvIndex)
        {
            cmdList->SetGraphicsRootDescriptorTable(rootIndex, env.srvManager->GetSRVHandleGPU(srvIndex));
        };

    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, material.resource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(5, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(6, settingsBuffer_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(7, chunkBuffer_->GetGPUVirtualAddress());

    SetTexture(8, material.albedoMapHandle);
    SetTexture(9, material.environmentMapHandle);
    if (shadowMap)
    {
        cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
    }
    SetTexture(11, material.normalMapHandle);
    SetTexture(12, material.rippleMapHandle);
    SetTexture(13, drawData_.heightMapHandle);

    cmdList->IASetVertexBuffers(0, 1, &drawData_.vertexBufferView);
    cmdList->IASetIndexBuffer(&drawData_.indexBufferView);
    cmdList->DrawIndexedInstanced(drawData_.indexCount, chunkCount_, 0, 0, 0);
}

void TerrainRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex)
{
    if (!isSubmitted_ || chunkCount_ == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    PIXScopedEvent(cmdList, FS::PIXColors::Shadow, "Terrain Shadow Pass (Cascade %u)", cascadeIndex);

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTerrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTerrain"));

    cmdList->SetGraphicsRootConstantBufferView(0, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRoot32BitConstant(1, cascadeIndex, 0);
    cmdList->SetGraphicsRootShaderResourceView(2, chunkBuffer_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, settingsBuffer_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(drawData_.heightMapHandle));

    // 視錐台で選んだチャンクを影にも使う
    cmdList->IASetVertexBuffers(0, 1, &drawData_.vertexBufferView);
    cmdList->IASetIndexBuffer(&drawData_.indexBufferView);
    cmdList->DrawIndexedInstanced(drawData_.indexCount, chunkCount_, 0, 0, 0);
}

}
