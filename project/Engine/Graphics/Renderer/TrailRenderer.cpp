#include "pch.h"
#include "TrailRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"
#include "PIXColors.h"

namespace FE
{

void TrailRenderer::Initialize(const RenderEnvironment& env)
{
    const uint32_t kMaxTotalTrailVertices = kMaxTrailCount * kMaxTrailVertices * 2;
    trailBatch_.verticesCPU.reserve(kMaxTotalTrailVertices);

    std::vector<TrailVertexData> dummyVertices(kMaxTotalTrailVertices);
    trailBatch_.mesh.InitializeVertexTrail(env.device->GetDevice(), dummyVertices);

    trailBatch_.materialResource = BufferManager::CreateMappedConstantBufferArray(
        env.device->GetDevice(),
        kMaxTrailCount,
        &trailBatch_.mappedMaterial
    );

    trailBatch_.wvpResource = BufferManager::CreateMappedConstantBuffer(
        env.device->GetDevice(),
        &trailBatch_.mappedWvp
    );
}

void TrailRenderer::BeginFrame()
{
    prevTrailCount_ = indexTrail_;
    indexTrail_ = 0;

    // クリア処理
    trailBatch_.verticesCPU.clear();
    trailBatches_.clear();
}

void TrailRenderer::Submit(const std::vector<TrailPoint>& points, const TrailModule& config, const Vector3& cameraPosition, float instanceSeed)
{
    if (indexTrail_ >= kMaxTrailCount) return;
    if (points.size() < 2) return;

    uint32_t textureHandle = TextureManager::GetInstance().Get(config.textureName);

    // マテリアル定数バッファデータの構築
    TrailMaterialData currentMatData{};
    currentMatData.jitterStrength = config.jitterStrength;
    currentMatData.jitterFrequency = config.jitterFrequency;
    currentMatData.jitterSpeed = config.jitterSpeed;
    currentMatData.jitterMode = static_cast<int>(config.jitterMode);
    currentMatData.jitterPhase = config.jitterPhase;
    currentMatData.emissiveIntensity = config.emissiveIntensity;
    currentMatData.instanceSeed = instanceSeed;

    // バッチマージの判定
    bool isNewBatch = trailBatches_.empty();
    if (!isNewBatch)
    {
        const auto& last = trailBatches_.back();
        isNewBatch = last.textureHandle != textureHandle ||
            std::memcmp(&last.materialData, &currentMatData, sizeof(TrailMaterialData)) != 0;
    }

    if (isNewBatch)
    {
        trailBatches_.push_back({ static_cast<uint32_t>(trailBatch_.verticesCPU.size()), 0, textureHandle, currentMatData });
    }

    // 各トレイルポイントの左右頂点位置・UV・カラーを一括計算
    struct TempVertex
    {
        Vector3 left;
        Vector3 right;
        float u;
        Vector4 color;
    };
    std::vector<TempVertex> calculatedVerts(points.size());
    const float lastIndexF = static_cast<float>(points.size() - 1);

    for (size_t i = 0; i < points.size(); ++i)
    {
        const Vector3& pos = points[i].position;

        Vector3 forward = (i < points.size() - 1) ? (points[i + 1].position - pos) : (pos - points[i - 1].position);
        forward = forward.Normalize();

        Vector3 right;
        if (config.alignment == TrailAlignment::View)
        {
            Vector3 toCamera = (cameraPosition - pos).Normalize();
            right = Math::CrossProduct(toCamera, forward).Normalize();
        }
        else
        {
            Vector3 up = points[i].rotationQuaternion.RotateVector({ 0.0f, 1.0f, 0.0f });
            right = Math::CrossProduct(up, forward).Normalize();
        }

        if (right.LengthSq() < 0.001f)
        {
            right = Math::CrossProduct({ 0.0f, 1.0f, 0.0f }, forward).Normalize();
        }

        float t = static_cast<float>(i) / lastIndexF;
        float width = config.width * std::lerp(config.tailWidthScale, config.headWidthScale, t);

        calculatedVerts[i].left = pos - right * (width * 0.5f);
        calculatedVerts[i].right = pos + right * (width * 0.5f);
        calculatedVerts[i].u = t;

        calculatedVerts[i].color = Vector4(
            std::lerp(config.endColor.x, config.startColor.x, t),
            std::lerp(config.endColor.y, config.startColor.y, t),
            std::lerp(config.endColor.z, config.startColor.z, t),
            std::lerp(config.endColor.w, config.startColor.w, t)
        );
    }

    // ポリゴン追加
    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        const auto& v0 = calculatedVerts[i];
        const auto& v1 = calculatedVerts[i + 1];

        trailBatch_.verticesCPU.push_back({ { v0.left.x,  v0.left.y,  v0.left.z,  1.0f }, { v0.u, 0.0f }, v0.color });
        trailBatch_.verticesCPU.push_back({ { v1.left.x,  v1.left.y,  v1.left.z,  1.0f }, { v1.u, 0.0f }, v1.color });
        trailBatch_.verticesCPU.push_back({ { v0.right.x, v0.right.y, v0.right.z, 1.0f }, { v0.u, 1.0f }, v0.color });

        trailBatch_.verticesCPU.push_back({ { v0.right.x, v0.right.y, v0.right.z, 1.0f }, { v0.u, 1.0f }, v0.color });
        trailBatch_.verticesCPU.push_back({ { v1.left.x,  v1.left.y,  v1.left.z,  1.0f }, { v1.u, 0.0f }, v1.color });
        trailBatch_.verticesCPU.push_back({ { v1.right.x, v1.right.y, v1.right.z, 1.0f }, { v1.u, 1.0f }, v1.color });

        trailBatches_.back().vertexCount += 6;
    }

    indexTrail_++;
}

void TrailRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewProjection)
{
    if (trailBatches_.empty() || trailBatch_.verticesCPU.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();

    // トレイルパス全体のスコープ
    PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Trail Pass");

    TrailVertexData* mappedVertices = nullptr;
    trailBatch_.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));
    memcpy(mappedVertices, trailBatch_.verticesCPU.data(), sizeof(TrailVertexData) * trailBatch_.verticesCPU.size());
    trailBatch_.mesh.GetVertexResource()->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW vbView = trailBatch_.mesh.GetVertexBufferView();
    vbView.SizeInBytes = static_cast<uint32_t>(sizeof(TrailVertexData) * trailBatch_.verticesCPU.size());
    cmdList->IASetVertexBuffers(0, 1, &vbView);

    cmdList->SetPipelineState(env.psoManager->GetPSO("Trail"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Trail"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 引数の viewProjection を使用
    trailBatch_.mappedWvp->WVP = viewProjection;
    trailBatch_.mappedWvp->World = Matrix4x4::MakeIdentity();
    cmdList->SetGraphicsRootConstantBufferView(0, trailBatch_.wvpResource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.globalConstants->GetResource()->GetGPUVirtualAddress());

    const uint32_t alignedSize = (sizeof(TrailMaterialData) + 255) & ~255;
    D3D12_GPU_VIRTUAL_ADDRESS materialBaseAddr = trailBatch_.materialResource->GetGPUVirtualAddress();
    uint8_t* mappedBasePtr = reinterpret_cast<uint8_t*>(trailBatch_.mappedMaterial);

    for (size_t i = 0; i < trailBatches_.size(); ++i)
    {
        const auto& batch = trailBatches_[i];
        if (batch.vertexCount == 0) continue;

        // バッチごとの個別スコープ（インデックスと頂点数を可視化）
        PIXScopedEvent(cmdList, FE::PIXColors::Geometry, "Trail Batch %zu (Vertices: %u)", i, batch.vertexCount);

        const uint32_t offset = static_cast<uint32_t>(i) * alignedSize;

        memcpy(mappedBasePtr + offset, &batch.materialData, sizeof(TrailMaterialData));
        cmdList->SetGraphicsRootConstantBufferView(1, materialBaseAddr + offset);

        cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(batch.textureHandle));

        cmdList->DrawInstanced(batch.vertexCount, 1, batch.startVertexIndex, 0);
    }
}

}