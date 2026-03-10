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

void TrailRenderer::Initialize(const RenderEnvironment& env)
{
    const uint32_t kMaxTotalTrailVertices = kMaxTrailCount * kMaxTrailVertices * 2;
    trailBatch_.verticesCPU.reserve(kMaxTotalTrailVertices);

    std::vector<VertexDataTrail> dummyVertices(kMaxTotalTrailVertices);
    trailBatch_.mesh.InitializeVertexTrail(env.device->GetDevice(), dummyVertices);

    uint32_t materialSize = sizeof(TrailMaterialData);
    materialSize = (materialSize + 255) & ~255;

    trailBatch_.materialResource = BufferManager::CreateBufferResource(
        env.device->GetDevice(), materialSize * kMaxTrailCount);
    trailBatch_.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&trailBatch_.mappedMaterial));

    trailBatch_.wvpResource = BufferManager::CreateBufferResource(
        env.device->GetDevice(), sizeof(TransformationMatrix));
    trailBatch_.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&trailBatch_.mappedWvp));
}

void TrailRenderer::BeginFrame()
{
    prevTrailCount_ = indexTrail_;
    indexTrail_ = 0;

    // クリア処理
    trailBatch_.verticesCPU.clear();
    trailBatches_.clear();
}

void TrailRenderer::Submit(const std::vector<TrailPoint>& points, const TrailModule& config, const Vector3& cameraPosition,         // 共通データ（カメラ位置）
    float instanceSeed)
{
    if (indexTrail_ >= kMaxTrailCount) return;
    if (points.size() < 2) return;

    // Renderer::SubmitTrailの中身をそのままコピー
    uint32_t textureHandle = TextureManager::GetInstance().Get(config.textureName);
    uint32_t dissolveHandle = (!config.dissolveTextureName.empty() && config.dissolveTextureName != "none")
        ? TextureManager::GetInstance().Get(config.dissolveTextureName)
        : TextureManager::GetInstance().Get("white1x1");

    TrailMaterialData currentMatData{};
    currentMatData.scrollSpeed = config.scrollSpeed;
    currentMatData.jitterStrength = config.jitterStrength;
    currentMatData.jitterFrequency = config.jitterFrequency;
    currentMatData.jitterSpeed = config.jitterSpeed;
    currentMatData.jitterMode = static_cast<int>(config.jitterMode);
    currentMatData.jitterPhase = config.jitterPhase;
    currentMatData.isDissolveEnabled = (config.dissolveTextureName != "white1x1") ? 1 : 0;
    currentMatData.emissiveIntensity = config.emissiveIntensity;
    currentMatData.instanceSeed = instanceSeed;

    bool isNewBatch = trailBatches_.empty();
    if (!isNewBatch)
    {
        const auto& last = trailBatches_.back();
        isNewBatch = last.textureHandle != textureHandle || last.dissolveHandle != dissolveHandle ||
            std::memcmp(&last.materialData, &currentMatData, sizeof(TrailMaterialData)) != 0;
    }

    if (isNewBatch)
    {
        trailBatches_.push_back({ static_cast<uint32_t>(trailBatch_.verticesCPU.size()), 0, textureHandle, dissolveHandle, currentMatData });
    }

    std::vector<float> distances;
    if (config.textureMode == TrailTextureMode::Tile)
    {
        distances.resize(points.size());
        float total = 0.0f;
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            distances[i] = total;
            total += (points[i + 1].position - points[i].position).Length();
        }
        distances.back() = total;
    }

    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        auto CalcVertex = [&](size_t idx, float& outU) {
            const Vector3& pos = points[idx].position;
            Vector3 forward = (idx < points.size() - 1) ? points[idx + 1].position - pos : pos - points[idx - 1].position;
            forward = forward.Normalize();

            Vector3 right;
            if (config.alignment == TrailAlignment::View)
            {
                // cameraPositionを使用
                Vector3 toCamera = (cameraPosition - pos).Normalize();
                right = Math::CrossProduct(toCamera, forward).Normalize();
            }
            else
            {
                Vector3 up = points[idx].rotationQuaternion.RotateVector({ 0,1,0 });
                right = Math::CrossProduct(up, forward).Normalize();
            }
            if (right.LengthSq() < 0.001f) right = Math::CrossProduct({ 0,1,0 }, forward).Normalize();

            float t = static_cast<float>(idx) / (points.size() - 1);
            float width = config.width * std::lerp(config.tailWidthScale, config.headWidthScale, t);

            outU = (config.textureMode == TrailTextureMode::Stretch) ? t * config.tiling.x : distances[idx] * config.tiling.x;
            return std::pair(pos - right * (width * 0.5f), pos + right * (width * 0.5f));
            };

        auto CalcColor = [&](size_t idx) {
            float t = static_cast<float>(idx) / (points.size() - 1);
            return Vector4(
                std::lerp(config.endColor.x, config.startColor.x, t),
                std::lerp(config.endColor.y, config.startColor.y, t),
                std::lerp(config.endColor.z, config.startColor.z, t),
                std::lerp(config.endColor.w, config.startColor.w, t));
            };

        float u0, u1;
        auto [l0, r0] = CalcVertex(i, u0);
        auto [l1, r1] = CalcVertex(i + 1, u1);
        Vector4 c0 = CalcColor(i);
        Vector4 c1 = CalcColor(i + 1);

        trailBatch_.verticesCPU.push_back({ {l0.x,l0.y,l0.z,1}, {u0,0}, c0 });
        trailBatch_.verticesCPU.push_back({ {l1.x,l1.y,l1.z,1}, {u1,0}, c1 });
        trailBatch_.verticesCPU.push_back({ {r0.x,r0.y,r0.z,1}, {u0,1}, c0 });

        trailBatch_.verticesCPU.push_back({ {r0.x,r0.y,r0.z,1}, {u0,1}, c0 });
        trailBatch_.verticesCPU.push_back({ {l1.x,l1.y,l1.z,1}, {u1,0}, c1 });
        trailBatch_.verticesCPU.push_back({ {r1.x,r1.y,r1.z,1}, {u1,1}, c1 });

        trailBatches_.back().vertexCount += 6;
    }
    indexTrail_++;
}

void TrailRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewProjection)
{
    if (trailBatches_.empty() || trailBatch_.verticesCPU.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();

    VertexDataTrail* mappedVertices = nullptr;
    trailBatch_.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));
    memcpy(mappedVertices, trailBatch_.verticesCPU.data(), sizeof(VertexDataTrail) * trailBatch_.verticesCPU.size());
    trailBatch_.mesh.GetVertexResource()->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW vbView = trailBatch_.mesh.GetVertexBufferView();
    vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDataTrail) * trailBatch_.verticesCPU.size());
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

        const uint32_t offset = static_cast<uint32_t>(i) * alignedSize;

        memcpy(mappedBasePtr + offset, &batch.materialData, sizeof(TrailMaterialData));
        cmdList->SetGraphicsRootConstantBufferView(1, materialBaseAddr + offset);

        cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(batch.textureHandle));
        uint32_t maskHandle = batch.materialData.isDissolveEnabled ? batch.dissolveHandle : batch.textureHandle;
        cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(maskHandle));

        cmdList->DrawInstanced(batch.vertexCount, 1, batch.startVertexIndex, 0);
    }
}