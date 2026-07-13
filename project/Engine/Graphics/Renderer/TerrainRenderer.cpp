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

    // 地形パラメータ用バッファ
    terrainSettingsBuffer_ = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TerrainSettings));
    terrainSettingsBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&terrainSettingsMapped_));

    // 初期値をセット 
    terrainSettingsMapped_->maxHeight = 100.0f;
    terrainSettingsMapped_->texelSize = 1.0f / 512.0f;
    terrainSettingsMapped_->cellSize = 1.0f; 

    // インスタンス配列用バッファ
    // 最大数分のInstanceDataを格納できる大きなバッファを1つだけ作る
    size_t instanceBufferSize = sizeof(TerrainInstanceData) * kMaxCount;
    instanceBuffer_ = BufferManager::CreateBufferResource(device_->GetDevice(), instanceBufferSize);
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceMapped_));
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
    const MaterialHandle& material, const Vector4& instanceColor,
    const Terrain::Parameters& params, uint32_t heightMapHandle)
{
    if (indexChunk_ >= kMaxCount || !chunk) return;

    TerrainSubmission submission{};
    submission.chunk = chunk;
    submission.materialHandle = material;
    submission.worldMatrix = worldTransform.matWorld_;
    submission.wvpMatrix = worldTransform.matWorld_ * viewProjectionMatrix_;
    submission.worldInverseTranspose = Matrix4x4::Inverse(submission.worldMatrix.Transpose());
    submission.instancingColor = instanceColor;
    submission.group = RenderGroup::Opaque;

    // 渡された地形パラメータとハイトマップを保存
    submission.params = params;
    submission.heightMapHandle = heightMapHandle;

    Matrix4x4 worldView = submission.worldMatrix * viewMatrix_;
    submission.depth = worldView.m[3][2];

    submissions_.push_back(submission);
    indexChunk_++;
}

void TerrainRenderer::PrepareBatches()
{
    if (submissions_.empty()) return;

    // グループ順 ＞ ハイトマップ順 ＞ マテリアル順 でソート
    std::sort(submissions_.begin(), submissions_.end(),
        [](const TerrainSubmission& a, const TerrainSubmission& b) {
            if (a.group != b.group) return a.group < b.group;
            if (a.group == RenderGroup::Transparent) return a.depth > b.depth;

            // ハイトマップのハンドルでソート（同じハイトマップを連続させる）
            if (a.heightMapHandle != b.heightMapHandle) return a.heightMapHandle < b.heightMapHandle;

            return a.materialHandle.materialData < b.materialHandle.materialData;
        });

    // GPUバッファへの書き込み処理はそのまま
    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        instanceMapped_[i].WVP = submissions_[i].wvpMatrix;
        instanceMapped_[i].World = submissions_[i].worldMatrix;
        instanceMapped_[i].WorldInverseTranspose = submissions_[i].worldInverseTranspose;
        instanceMapped_[i].WorldColor = submissions_[i].instancingColor;
        submissions_[i].instanceIndex = static_cast<uint32_t>(i);
    }
}

void TerrainRenderer::Draw(const RenderEnvironment & env, RenderGroup targetGroup, ShadowMap * shadowMap)
{
    if (submissions_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Terrain"));

    ID3D12PipelineState* pso = env.psoManager->GetPSO("Terrain");
    cmdList->SetPipelineState(pso);

    // 全チャンク共通の定数バッファをセット (ループ外)
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); // b0
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress()); // b1
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress()); // b2
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress()); // b3
    cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetAreaLightResource()->GetGPUVirtualAddress()); // b4
    cmdList->SetGraphicsRootConstantBufferView(6, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress()); // b8

    // 最新のパラメータを書き込む
    terrainSettingsMapped_->maxHeight = submissions_[0].params.maxHeight;
    terrainSettingsMapped_->texelSize = submissions_[0].params.texelSize;
    terrainSettingsMapped_->cellSize = submissions_[0].params.cellSize;

    cmdList->SetGraphicsRootConstantBufferView(7, terrainSettingsBuffer_->GetGPUVirtualAddress());// b10
    cmdList->SetGraphicsRootShaderResourceView(8, instanceBuffer_->GetGPUVirtualAddress());

    // バッチ描画の準備
    const MaterialData* currentMaterial = nullptr;
    const TerrainChunk* currentChunk = nullptr;
    uint32_t currentHeightMap = 0;
    uint32_t instanceStart = 0;
    uint32_t instanceCount = 0;

    // バッチ（溜まったインスタンス）を一気に描画するラムダ式
    auto FlushBatch = [&]()
        {
        if (instanceCount > 0 && currentChunk)
        {
            cmdList->DrawIndexedInstanced(currentChunk->GetIndexCount(), instanceCount, 0, 0, instanceStart);
        }
        };

    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];
        if (sub.group != targetGroup) continue;

        // チャンク形状、マテリアル、ハイトマップのいずれかが変わったらバッチを区切る
        if (currentMaterial != sub.materialHandle.materialData ||
            currentChunk != sub.chunk ||
            currentHeightMap != sub.heightMapHandle)
        {
            FlushBatch();

            // チャンク(VB/IB)の更新
            if (currentChunk != sub.chunk)
            {
                currentChunk = sub.chunk;
                D3D12_VERTEX_BUFFER_VIEW vbv = currentChunk->GetVertexBufferView();
                D3D12_INDEX_BUFFER_VIEW ibv = currentChunk->GetIndexBufferView();
                cmdList->IASetVertexBuffers(0, 1, &vbv);
                cmdList->IASetIndexBuffer(&ibv);
            }

            // ハイトマップテクスチャの更新
            if (currentHeightMap != sub.heightMapHandle)
            {
                currentHeightMap = sub.heightMapHandle;
                cmdList->SetGraphicsRootDescriptorTable(17, env.srvManager->GetSRVHandleGPU(currentHeightMap));
            }

            // マテリアルの更新
            if (currentMaterial != sub.materialHandle.materialData)
            {
                currentMaterial = sub.materialHandle.materialData;
                cmdList->SetGraphicsRootConstantBufferView(5, sub.materialHandle.resource->GetGPUVirtualAddress());

                cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(sub.materialHandle.textureHandle));       // t0
                cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(sub.materialHandle.envMapHandle));        // t1
                if (shadowMap) {
                    cmdList->SetGraphicsRootDescriptorTable(11, shadowMap->GetSRVHandle());                                           // t2
                }
                cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(sub.materialHandle.toonRampHandle));       // t3
                cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(sub.materialHandle.dissolveMapHandle));    // t4
                cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(sub.materialHandle.normalMapHandle));      // t5
                cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(sub.materialHandle.rippleTextureHandle));  // t6
                cmdList->SetGraphicsRootDescriptorTable(16, env.srvManager->GetSRVHandleGPU(sub.materialHandle.puddleNoiseHandle));    // t7
            }

            instanceStart = sub.instanceIndex;
            instanceCount = 0;
        }

        instanceCount++;
    }

    // ループを抜けたら、最後のバッチを描画する
    FlushBatch();
}

void TerrainRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex)
{
    if (submissions_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // ハイトマップテクスチャを読み込むため、DescriptorHeap をセット
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapTerrain"));
    cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapTerrain"));

    // 全チャンク共通の設定 (ループの外で一度だけセット)
    cmdList->SetGraphicsRootConstantBufferView(0, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRoot32BitConstant(1, cascadeIndex, 0);
    cmdList->SetGraphicsRootShaderResourceView(2, instanceBuffer_->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, terrainSettingsBuffer_->GetGPUVirtualAddress());

    // バッチ描画のループ処理
    const TerrainChunk* currentChunk = nullptr;
    uint32_t currentHeightMap = 0;
    uint32_t instanceStart = 0;
    uint32_t instanceCount = 0;

    // バッチをまとめて描画するラムダ式
    auto FlushBatch = [&]()
        {
        if (instanceCount > 0 && currentChunk)
        {
            cmdList->DrawIndexedInstanced(currentChunk->GetIndexCount(), instanceCount, 0, 0, instanceStart);
        }
        };

    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];

        // シャドウ描画が不要なグループや、完全に透明なものはスキップ
        if (sub.group == RenderGroup::Background || sub.group == RenderGroup::UI) continue;
        if (sub.materialHandle.materialData->color.w <= 0.0f || sub.group == RenderGroup::Transparent) continue;

        // チャンク形状、またはハイトマップが変わったらバッチを区切って描画 (シャドウはマテリアルを無視)
        if (currentChunk != sub.chunk || currentHeightMap != sub.heightMapHandle)
        {
            FlushBatch();

            // チャンク(頂点・インデックスバッファ)の更新
            if (currentChunk != sub.chunk)
            {
                currentChunk = sub.chunk;
                D3D12_VERTEX_BUFFER_VIEW vbv = currentChunk->GetVertexBufferView();
                D3D12_INDEX_BUFFER_VIEW ibv = currentChunk->GetIndexBufferView();
                cmdList->IASetVertexBuffers(0, 1, &vbv);
                cmdList->IASetIndexBuffer(&ibv);
            }

            // ハイトマップテクスチャの更新
            if (currentHeightMap != sub.heightMapHandle)
            {
                currentHeightMap = sub.heightMapHandle;
                cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(currentHeightMap));
            }

            instanceStart = sub.instanceIndex;
            instanceCount = 0;
        }

        instanceCount++;
    }

    // ループを抜けたら、最後の残りバッチを描画
    FlushBatch();
}

}