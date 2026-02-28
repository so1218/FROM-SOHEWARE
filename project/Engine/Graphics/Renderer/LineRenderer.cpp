#include "LineRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"

void LineRenderer::Initialize(const RenderEnvironment& env)
{
    // 動的頂点バッファ作成
    lineBatch_.mesh.CreateDynamicMesh(env.device->GetDevice(), kMaxVertices, sizeof(LineVertex));
    lineBatch_.verticesCPU.reserve(kMaxVertices);

    // WVP用定数バッファ作成
    lineBatch_.wvpResource = BufferManager::CreateBufferResource(env.device->GetDevice(), sizeof(TransformationMatrix));
    lineBatch_.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&lineBatch_.mappedWvp));
}

void LineRenderer::BeginFrame() 
{
    prevCount_ = static_cast<uint32_t>(lineBatch_.verticesCPU.size() / 2);
    lineBatch_.verticesCPU.clear(); // 毎フレームクリア
}

void LineRenderer::Submit(const Vector3& start, const Vector3& end, uint32_t color) 
{
    if (lineBatch_.verticesCPU.size() >= kMaxVertices) return;

    Vector4 colorVec = Math::Uint32ToColorVector(color);
    lineBatch_.verticesCPU.push_back({ {start.x, start.y, start.z, 1.0f}, colorVec });
    lineBatch_.verticesCPU.push_back({ {end.x, end.y, end.z, 1.0f}, colorVec });
}

void LineRenderer::Draw(const RenderEnvironment& env, const Matrix4x4& viewProjection) 
{
    if (lineBatch_.verticesCPU.empty()) return;

    // 行列更新
    lineBatch_.mappedWvp->WVP = viewProjection;

    // GPUへコピー
    LineVertex* gpuPtr = nullptr;
    lineBatch_.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&gpuPtr));
    std::memcpy(gpuPtr, lineBatch_.verticesCPU.data(), sizeof(LineVertex) * lineBatch_.verticesCPU.size());
    lineBatch_.mesh.GetVertexResource()->Unmap(0, nullptr);

    auto* cmdList = env.commandManager->GetCommandList();

    // ステート設定
    cmdList->SetPipelineState(env.psoManager->GetPSO("Line"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Line"));

    // ラインリストとして描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    auto vbView = lineBatch_.mesh.GetVertexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    cmdList->SetGraphicsRootConstantBufferView(0, lineBatch_.wvpResource->GetGPUVirtualAddress());

    cmdList->DrawInstanced(static_cast<UINT>(lineBatch_.verticesCPU.size()), 1, 0, 0);
}