#include "pch.h"
#include "LightningRenderer.h"
#include "BufferManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "GlobalConstants.h"
#include "CommandManager.h"
#include "GraphicsDevice.h"
#include "PIXColors.h"

namespace FS
{

void LightningRenderer::Initialize(const RenderEnvironment& env)
{
    auto device = env.device->GetDevice();

    for (int i = 0; i < kFrameCount; ++i)
    {
        vertexBuffer_[i] = BufferManager::CreateMappedBuffer(device, kMaxVertices, &mappedVertices_[i]);
        indexBuffer_[i] = BufferManager::CreateMappedBuffer(device, kMaxIndices, &mappedIndices_[i]);
        materialResource_[i] = BufferManager::CreateMappedConstantBuffer(device, &mappedMaterial_[i]);
    }
}

void LightningRenderer::BeginFrame()
{
    currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
    submissions_.clear();
}

void LightningRenderer::Submit(const std::vector<LightningPath>& paths, float brightness)
{
    submissions_.push_back({ paths, brightness });
}

void LightningRenderer::Draw(const RenderEnvironment& env, const Vector3& cameraPosition)
{
    if (submissions_.empty()) return;

    LightningMaterial* material = mappedMaterial_[currentFrameIndex_];
    material->coreColor = config_.coreColor;
    material->coreThickness = config_.coreThickness;
    material->fringeColor = config_.fringeColor;
    material->corePower = config_.corePower;
    material->glowPower = config_.glowPower;
    material->emissiveIntensity = config_.emissiveIntensity;

    LightningVertexData* vertices = mappedVertices_[currentFrameIndex_];
    uint32_t* indices = mappedIndices_[currentFrameIndex_];
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    for (const Submission& submission : submissions_)
    {
        for (const LightningPath& path : submission.paths)
        {
            const std::vector<Vector3>& points = path.points;
            const uint32_t pointCount = static_cast<uint32_t>(points.size());
            if (pointCount < 2) continue;
            if (vertexCount + pointCount * 2 > kMaxVertices || indexCount + (pointCount - 1) * 6 > kMaxIndices) break;

            const float halfWidth = config_.thickness * path.thicknessScale * 0.5f;
            const uint32_t baseVertex = vertexCount;

            for (uint32_t i = 0; i < pointCount; ++i)
            {
                const Vector3& point = points[i];
                Vector3 direction = (i + 1 < pointCount) ? points[i + 1] - point : point - points[i - 1];

                // 進む向きとカメラへの向きの両方に直角な向きへ広げると、板がカメラを向く
                Vector3 side = direction.Cross(cameraPosition - point);
                float sideLength = side.Length();
                if (sideLength < 1e-5f) continue;
                side = side * (halfWidth / sideLength);

                float heightRatio = static_cast<float>(i) / static_cast<float>(pointCount - 1);
                Vector3 left = point - side;
                Vector3 right = point + side;
                vertices[vertexCount++] = { Vector4(left.x, left.y, left.z, 1.0f), { 0.0f, heightRatio }, submission.brightness };
                vertices[vertexCount++] = { Vector4(right.x, right.y, right.z, 1.0f), { 1.0f, heightRatio }, submission.brightness };
            }

            const uint32_t emittedPointCount = (vertexCount - baseVertex) / 2;
            for (uint32_t i = 0; i + 1 < emittedPointCount; ++i)
            {
                uint32_t corner = baseVertex + i * 2;
                indices[indexCount++] = corner;
                indices[indexCount++] = corner + 1;
                indices[indexCount++] = corner + 2;
                indices[indexCount++] = corner + 1;
                indices[indexCount++] = corner + 3;
                indices[indexCount++] = corner + 2;
            }
        }
    }

    if (indexCount == 0) return;

    auto* commandList = env.commandManager->GetCommandList();

    PIXScopedEvent(commandList, FS::PIXColors::Geometry,
        "Lightning Pass (Strikes: %zu, Vertices: %u, Indices: %u)",
        submissions_.size(), vertexCount, indexCount);

    commandList->SetPipelineState(env.psoManager->GetPSO("Lightning"));
    commandList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Lightning"));

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    vertexBufferView.BufferLocation = vertexBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = vertexCount * sizeof(LightningVertexData);
    vertexBufferView.StrideInBytes = sizeof(LightningVertexData);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    indexBufferView.BufferLocation = indexBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = indexCount * sizeof(uint32_t);
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    commandList->IASetIndexBuffer(&indexBufferView);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_[currentFrameIndex_]->GetGPUVirtualAddress());
    commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

}
