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
    InitializeBatch(env, main_);
    InitializeBatch(env, vista_);
}

void TerrainRenderer::InitializeBatch(const RenderEnvironment& env, Batch& batch)
{
    batch.settingsBuffer = BufferManager::CreateMappedConstantBuffer(env.device->GetDevice(), &batch.mappedSettings);
    *batch.mappedSettings = {};

    batch.chunkBuffer = BufferManager::CreateMappedBuffer(env.device->GetDevice(), kMaxChunkCount, &batch.mappedChunks);
}

void TerrainRenderer::BeginFrame()
{
    main_.isSubmitted = false;
    main_.chunkCount = 0;
    vista_.isSubmitted = false;
    vista_.chunkCount = 0;
}

void TerrainRenderer::Submit(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks)
{
    SubmitBatch(main_, drawData, visibleChunks);
}

void TerrainRenderer::SubmitVista(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks)
{
    SubmitBatch(vista_, drawData, visibleChunks);
}

void TerrainRenderer::SubmitBatch(Batch& batch, const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks)
{
    batch.drawData = drawData;
    batch.isSubmitted = true;

    // GPU が描き終わるのを毎フレーム待っているので、ここで直接書き換え
    *batch.mappedSettings = drawData.settings;

    batch.chunkCount = static_cast<uint32_t>(std::min<size_t>(visibleChunks.size(), kMaxChunkCount));
    if (batch.chunkCount > 0)
    {
        std::memcpy(batch.mappedChunks, visibleChunks.data(), sizeof(TerrainChunkData) * batch.chunkCount);
    }
}

void TerrainRenderer::Draw(const RenderEnvironment& env, ShadowMap* shadowMap, uint32_t fireMapSrvIndex, D3D12_GPU_VIRTUAL_ADDRESS fireCBAddress)
{
    // 遠景は歩ける地形の下に隠れる所があるので、歩ける地形を先に描いて深度で省く
    DrawBatch(env, shadowMap, main_, "Terrain Pass", fireMapSrvIndex, fireCBAddress);
    DrawBatch(env, shadowMap, vista_, "Terrain Vista Pass", fireMapSrvIndex, fireCBAddress);
}

void TerrainRenderer::DrawBatch(const RenderEnvironment& env, ShadowMap* shadowMap, const Batch& batch, const char* eventName,
    uint32_t fireMapSrvIndex, D3D12_GPU_VIRTUAL_ADDRESS fireCBAddress)
{
    if (!batch.isSubmitted || batch.chunkCount == 0 || !batch.drawData.material) return;

    auto* cmdList = env.commandManager->GetCommandList();
    PIXScopedEvent(cmdList, FS::PIXColors::Geometry, "%s (Chunks: %u)", eventName, batch.chunkCount);

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Terrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("Terrain"));

    const TerrainMaterial& material = *batch.drawData.material;
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
    cmdList->SetGraphicsRootConstantBufferView(6, batch.settingsBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(7, batch.chunkBuffer->GetGPUVirtualAddress());

    SetTexture(8, material.albedoMapHandle);
    SetTexture(9, material.environmentMapHandle);
    if (shadowMap)
    {
        cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
    }
    SetTexture(11, material.normalMapHandle);
    SetTexture(12, material.rippleMapHandle);
    SetTexture(13, batch.drawData.heightMapHandle);
    SetTexture(14, fireMapSrvIndex);
    cmdList->SetGraphicsRootConstantBufferView(15, fireCBAddress);

    cmdList->IASetVertexBuffers(0, 1, &batch.drawData.vertexBufferView);
    cmdList->IASetIndexBuffer(&batch.drawData.indexBufferView);
    cmdList->DrawIndexedInstanced(batch.drawData.indexCount, batch.chunkCount, 0, 0, 0);
}

void TerrainRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex)
{
    // 遠景はほとんど影の範囲の外にあるので、影は落とさない
    if (!main_.isSubmitted || main_.chunkCount == 0) return;

    auto* cmdList = env.commandManager->GetCommandList();
    PIXScopedEvent(cmdList, FS::PIXColors::Shadow, "Terrain Shadow Pass (Cascade %u)", cascadeIndex);

    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTerrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTerrain"));

    cmdList->SetGraphicsRootConstantBufferView(0, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRoot32BitConstant(1, cascadeIndex, 0);
    cmdList->SetGraphicsRootShaderResourceView(2, main_.chunkBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, main_.settingsBuffer->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(main_.drawData.heightMapHandle));

    // 視錐台で選んだチャンクを影にも使う
    cmdList->IASetVertexBuffers(0, 1, &main_.drawData.vertexBufferView);
    cmdList->IASetIndexBuffer(&main_.drawData.indexBufferView);
    cmdList->DrawIndexedInstanced(main_.drawData.indexCount, main_.chunkCount, 0, 0, 0);
}

}
