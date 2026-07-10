#include "pch.h"
#include "TerrainRenderer.h"
#include "TerrainChunk.h"
#include "ShadowMap.h"
#include "BufferManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "CommandManager.h"
#include "GraphicsDevice.h"

namespace FE
{

void TerrainRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device;

    // チャンクごとの定数バッファを作成（BufferManagerを活用）
    perObjectBuffers_.resize(kMaxCount);
    for (auto& buffer : perObjectBuffers_)
    {
        buffer.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
        buffer.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.wvpMapped));
    }
}

void TerrainRenderer::Finalize()
{

}

void TerrainRenderer::BeginFrame()
{
    prevCount_ = indexChunk_;
    indexChunk_ = 0;
    submissions_.clear();
}

void TerrainRenderer::SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection)
{
    viewMatrix_ = view;
    viewProjectionMatrix_ = viewProjection;
}

void TerrainRenderer::Submit(const WorldTransform& worldTransform, const TerrainChunk* chunk,
    const MaterialHandle& material, const Vector4& instanceColor)
{
    if (indexChunk_ >= kMaxCount || !chunk) return;

    auto& buffer = perObjectBuffers_[indexChunk_];

    // 行列計算
    Matrix4x4 world = worldTransform.matWorld_;
    Matrix4x4 wvp = world * viewProjectionMatrix_;

    // 定数バッファへ書き込み
    buffer.wvpMapped->WVP = wvp;
    buffer.wvpMapped->World = world;
    buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
    buffer.wvpMapped->WorldColor = instanceColor;

    TerrainSubmission submission{};
    submission.chunk = chunk;
    submission.materialHandle = material;
    submission.worldMatrix = world;
    submission.wvpMatrix = wvp;
    submission.worldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
    submission.instancingColor = instanceColor;

    submission.instanceIndex = indexChunk_;
    submission.blendMode = BlendMode::kBlendModeNone;
    submission.cullMode = CullMode::Back;
    submission.depthMode = DepthMode::Write;
    submission.group = RenderGroup::Opaque;

    // 深度計算 (カメラからの距離)
    Matrix4x4 worldView = world * viewMatrix_;
    submission.depth = worldView.m[3][2];

    submissions_.push_back(submission);
    indexChunk_++;
}

void TerrainRenderer::PrepareBatches()
{
    if (submissions_.empty()) return;

    // 半透明地形のために奥から手前へソート、不透明はマテリアル順でステート変更を抑える
    std::sort(submissions_.begin(), submissions_.end(),
        [](const TerrainSubmission& a, const TerrainSubmission& b) {
            if (a.group != b.group) return a.group < b.group;
            if (a.group == RenderGroup::Transparent) return a.depth > b.depth;
            return a.materialHandle.materialData < b.materialHandle.materialData;
        });
}

void TerrainRenderer::Draw(const RenderEnvironment& env, RenderGroup targetGroup, bool isWireFrame, ShadowMap* shadowMap)
{
    if (submissions_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 地形用のルートシグネチャ
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Terrain"));

    for (const auto& sub : submissions_)
    {
        if (sub.group != targetGroup) continue;

        ID3D12PipelineState* pso = env.psoManager->GetPSO("Terrain");
        cmdList->SetPipelineState(pso);

        // バッファのセット (TerrainChunkからViewを取得する前提)
        D3D12_VERTEX_BUFFER_VIEW vbv = sub.chunk->GetVertexBufferView();
        D3D12_INDEX_BUFFER_VIEW ibv = sub.chunk->GetIndexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);

        auto& buffer = perObjectBuffers_[sub.instanceIndex];

        cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetAreaLightResource()->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(5, sub.materialHandle.resource->GetGPUVirtualAddress());

        // Terrainの変換行列 (b6に対応するのはインデックス6)
        cmdList->SetGraphicsRootConstantBufferView(6, buffer.wvpResource->GetGPUVirtualAddress());

        // シャドウデータ (b8に対応するのはインデックス7)
        cmdList->SetGraphicsRootConstantBufferView(7, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());


        // ====================================================================
        // テクスチャ (Descriptor Table) のセット : インデックス 8 ～ 15
        // ====================================================================
        cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(sub.materialHandle.textureHandle));
        cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(sub.materialHandle.envMapHandle));

        if (shadowMap) {
            cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
        }

        cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(sub.materialHandle.toonRampHandle));
        cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(sub.materialHandle.dissolveMapHandle));
        cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(sub.materialHandle.normalMapHandle));
        cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(sub.materialHandle.rippleTextureHandle));
        cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(sub.materialHandle.puddleNoiseHandle));

        // 描画コマンド
        cmdList->DrawIndexedInstanced(sub.chunk->GetIndexCount(), 1, 0, 0, 0);
    }
}

void TerrainRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex)
{
    if (submissions_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTerrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTerrain"));

    for (const auto& sub : submissions_)
    {
        if (sub.group == RenderGroup::Background || sub.group == RenderGroup::UI) continue;

        // 地形が半透明の場合は影を落とさない（または独自処理）
        if (sub.materialHandle.materialData->color.w <= 0.0f || sub.group == RenderGroup::Transparent) continue;

        D3D12_VERTEX_BUFFER_VIEW vbv = sub.chunk->GetVertexBufferView();
        D3D12_INDEX_BUFFER_VIEW ibv = sub.chunk->GetIndexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);

        auto& buffer = perObjectBuffers_[sub.instanceIndex];

        // ルートシグネチャ "TerrainShadow" の定義に合わせたバインド
        // [0] b6: 地形のTransform (World行列が含まれている)
        cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());

        // [1] b8: ShadowData (カスケードの行列群)
        cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

        // [2] b9: CascadeIndex
        cmdList->SetGraphicsRoot32BitConstant(2, cascadeIndex, 0);

        cmdList->DrawIndexedInstanced(sub.chunk->GetIndexCount(), 1, 0, 0, 0);
    }
}

}