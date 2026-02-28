#include "ModelRenderer.h"
#include "Renderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"

#include <algorithm>
#include <cassert>

void ModelRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device; 

    perObjectBuffers_.resize(kMaxModelCount);
    for (auto& buffer : perObjectBuffers_)
    {
        buffer.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
        buffer.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.wvpMapped));
    }
}

void ModelRenderer::Finalize()
{
    meshCache_.clear();
}

void ModelRenderer::BeginFrame()
{
    prevModelCount_ = indexModel_;
    indexModel_ = 0;
    modelSubmissions_.clear();
}

void ModelRenderer::SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection)
{
    viewMatrix_ = view;
    viewProjectionMatrix_ = viewProjection;
}

const std::vector<Mesh>& ModelRenderer::GetOrCreateModelBatch(const ModelData& modelData)
{
    auto it = meshCache_.find(&modelData);
    if (it != meshCache_.end()) return it->second.meshes;

    ModelBatch batch;
    batch.meshes.resize(modelData.meshes.size());

    for (size_t i = 0; i < modelData.meshes.size(); ++i)
    {
        batch.meshes[i].Initialize(device_->GetDevice(), modelData.meshes[i].vertices, modelData.meshes[i].indices);
        batch.meshes[i].SetVertexCount(static_cast<uint32_t>(modelData.meshes[i].vertices.size()));
        batch.meshes[i].SetIndexCount(static_cast<uint32_t>(modelData.meshes[i].indices.size()));
    }

    meshCache_[&modelData] = std::move(batch);
    return meshCache_[&modelData].meshes;
}

void ModelRenderer::SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
    const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
    DepthMode depthMode, RenderGroup group, const Vector4& instanceColor)
{
    // モデルに対応するGPUメッシュリストを取得
    const auto& meshes = GetOrCreateModelBatch(modelData);

    // 再帰的にノードを巡回するラムダ関数
    std::function<void(const Node&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix)
        {
            Matrix4x4 currentWorldMatrix = node.localMatrix * parentMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                if (indexModel_ >= kMaxModelCount) return; // 安全対策

                const auto& meshPart = modelData.meshes[meshIndex];

                MaterialHandle actualMaterialHandle;
                if (meshIndex < materials.size())
                {
                    actualMaterialHandle = materials[meshIndex];
                }
                else
                {
                    actualMaterialHandle = materials.empty() ? meshPart.materialHandle : materials[0];
                }

                uint32_t actualTextureHandle = actualMaterialHandle.textureHandle;
                if (actualTextureHandle == 0)
                {
                    actualTextureHandle = meshPart.textureData.textureHandle;
                }

                auto& buffer = perObjectBuffers_[indexModel_];

                // 行列計算
                Matrix4x4 wvp = currentWorldMatrix * viewProjectionMatrix_;
                buffer.wvpMapped->WVP = wvp;
                buffer.wvpMapped->World = currentWorldMatrix;
                buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(currentWorldMatrix.Transpose());
                buffer.wvpMapped->WorldColor = instanceColor;

                ModelSubmission submission{};
                submission.type = RenderType::Model;
                submission.group = group;
                submission.modelData = &modelData;
                submission.meshIndex = meshIndex;
                submission.materialHandle = actualMaterialHandle;
                submission.textureHandle = actualTextureHandle;
                submission.envMapSrvHandle = actualMaterialHandle.envMapHandle;
                submission.toonRampHandle = actualMaterialHandle.toonRampHandle;
                submission.dissolveTextureHandle = actualMaterialHandle.dissolveMapHandle;
                submission.normalMapHandle = actualMaterialHandle.normalMapHandle;
                submission.rippleTextureHandle = actualMaterialHandle.rippleTextureHandle;
                submission.puddleNoiseHandle = actualMaterialHandle.puddleNoiseHandle;
                submission.worldMatrix = currentWorldMatrix;

                if (actualMaterialHandle.materialData)
                {
                    submission.enableOutline = (actualMaterialHandle.materialData->enableOutline != 0);
                }
                else
                {
                    submission.enableOutline = false;
                }

                submission.instanceIndex = indexModel_;
                submission.blendMode = blendMode;
                submission.cullMode = cullMode;
                submission.depthMode = depthMode;

                // アルファ判定
                bool hasAlpha = (Math::ColorVectorToUint32(submission.materialHandle.materialData->color) & 0xFF) < 255;
                bool isBlend = submission.blendMode != BlendMode::kBlendModeNone;

                if (hasAlpha || isBlend)
                {
                    submission.group = RenderGroup::Transparent;
                    if (submission.blendMode == BlendMode::kBlendModeNone)
                    {
                        submission.blendMode = BlendMode::kBlendModeNormal;
                    }
                }
                else
                {
                    submission.group = group;
                }

                // 深度設定
                Matrix4x4 worldView = currentWorldMatrix * viewMatrix_;
                submission.depth = worldView.m[3][2];

                modelSubmissions_.push_back(submission);
                indexModel_++;
            }

            for (const auto& child : node.children)
            {
                Traverse(child, currentWorldMatrix);
            }
        };

    Traverse(modelData.rootNode, worldTransform.matWorld_);
}

void ModelRenderer::SubmitAnimationModel(
    const WorldTransform& worldTransform,
    const AnimatedModelData& instance,
    const SkinCluster& skinCluster,
    const std::vector<MaterialHandle>& materials,
    BlendMode blendMode,
    RenderGroup group,
    const Vector4& instanceColor)
{
    const ModelData* modelData = instance.modelData;
    // GPUメッシュ生成済みか確認
    GetOrCreateModelBatch(*modelData);

    for (size_t i = 0; i < modelData->meshes.size(); ++i)
    {
        assert(indexModel_ < kMaxModelCount);

        const auto& meshPart = modelData->meshes[i];
        auto& buffer = perObjectBuffers_[indexModel_];

        // 各パーツのWorld行列はモデル全体のWorldで統一
        Matrix4x4 world = worldTransform.matWorld_;
        Matrix4x4 wvp = world * viewProjectionMatrix_;
        buffer.wvpMapped->WVP = wvp;
        buffer.wvpMapped->World = world;
        buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
        buffer.wvpMapped->WorldColor = instanceColor;

        // マテリアル決定
        MaterialHandle actualMaterialHandle;
        if (i < materials.size())
        {
            actualMaterialHandle = materials[i];
        }
        else
        {
            // 万が一足りない場合は0番目かデフォルト
            actualMaterialHandle = materials.empty() ? meshPart.materialHandle : materials[0];
        }

        // マテリアルからテクスチャ情報を取得
        uint32_t actualTextureHandle = actualMaterialHandle.textureHandle;
        if (actualTextureHandle == 0)
        {
            actualTextureHandle = meshPart.textureData.textureHandle;
        }

        // 描画キュー登録
        ModelSubmission submission{};
        submission.type = RenderType::Skinning;
        submission.group = group;
        submission.modelData = modelData;
        submission.meshIndex = static_cast<uint32_t>(i); // 何番目のメッシュか指定
        submission.materialHandle = actualMaterialHandle;
        submission.textureHandle = actualTextureHandle;
        submission.envMapSrvHandle = actualMaterialHandle.envMapHandle;
        submission.toonRampHandle = actualMaterialHandle.toonRampHandle;
        submission.dissolveTextureHandle = actualMaterialHandle.dissolveMapHandle;
        submission.normalMapHandle = actualMaterialHandle.normalMapHandle;
        submission.rippleTextureHandle = actualMaterialHandle.rippleTextureHandle;
        submission.puddleNoiseHandle = actualMaterialHandle.puddleNoiseHandle;
        submission.worldMatrix = world;
        // マテリアルデータのポインタが存在し、かつenableOutlineがtrueなら有効
        if (actualMaterialHandle.materialData)
        {
            submission.enableOutline = (actualMaterialHandle.materialData->enableOutline != 0);
        }
        else {
            submission.enableOutline = false;
        }
        submission.instanceIndex = indexModel_;
        submission.skinCluster = &skinCluster;
        submission.blendMode = blendMode;

        // アルファ判定
        bool hasAlpha = (Math::ColorVectorToUint32(submission.materialHandle.materialData->color) & 0xFF) < 255;
        bool isBlend = submission.blendMode != BlendMode::kBlendModeNone;

        if (hasAlpha || isBlend)
        {
            submission.group = RenderGroup::Transparent;
            if (submission.blendMode == BlendMode::kBlendModeNone)
            {
                submission.blendMode = BlendMode::kBlendModeNormal;
            }
        }
        else
        {
            submission.group = group;
        }

        // 深度設定
        Matrix4x4 worldView = world * viewMatrix_;
        submission.depth = worldView.m[3][2];

        modelSubmissions_.push_back(submission);
        indexModel_++;
    }
}

void ModelRenderer::Draw(const RenderEnvironment& env, RenderGroup targetGroup, bool isWireFrame, ShadowMap* shadowMap)
{
    if (modelSubmissions_.empty()) return;

    std::sort(modelSubmissions_.begin(), modelSubmissions_.end(),
        [](const ModelSubmission& a, const ModelSubmission& b)
        {
            if (a.group == RenderGroup::Opaque) return a.depth < b.depth;
            return a.depth > b.depth;
        });

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const auto& sub : modelSubmissions_)
    {
        if (sub.group != targetGroup) continue;

        DrawModelCore(env, sub, isWireFrame, shadowMap);
    }
}

void ModelRenderer::DrawModelCore(const RenderEnvironment& env, const ModelSubmission& sub, bool isWireFrame, ShadowMap* shadowMap)
{
    const std::vector<Mesh>& meshes = GetOrCreateModelBatch(*sub.modelData);
    assert(sub.meshIndex < meshes.size());
    const Mesh* mesh = &meshes[sub.meshIndex];

    auto& buffer = perObjectBuffers_[sub.instanceIndex];
    auto* cmdList = env.commandManager->GetCommandList();
    bool isSkinning = (sub.skinCluster != nullptr);

    std::string psoName;

    if (isSkinning)
    {
        psoName = "Skinning";
    }
    else if (isWireFrame)
    {
        psoName = "Wireframe";
    }
    else
    {
        switch (sub.blendMode)
        {
        case BlendMode::kBlendModeAdd:      psoName = "Object3DAdd";         break;
        case BlendMode::kBlendModeNormal:   psoName = "Object3DTransparent"; break;
        case BlendMode::kBlendModeNone:
        default:                            psoName = "Standard3D";          break;
        }
    }

    if (psoName != "Wireframe")
    {
        if (sub.cullMode == CullMode::None || sub.cullMode == CullMode::Front) psoName += "_NoCull";
        if (sub.depthMode == DepthMode::ReadOnly) psoName += "_DepthRead";
        else if (sub.depthMode == DepthMode::None) psoName += "_DepthOff";
    }

    // アウトライン描画
    if (sub.enableOutline)
    {
        if (isSkinning)
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("SkinningOutline"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("SkinningOutline"));

            cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootDescriptorTable(1, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
            cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(3, env.globalConstants->GetResource()->GetGPUVirtualAddress());

            const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
            D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
            cmdList->IASetVertexBuffers(0, 2, vbvs);
        }
        else
        {
            cmdList->SetPipelineState(env.psoManager->GetPSO("Object3DOutline"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Outline"));

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(1, sub.materialHandle.resource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(2, env.globalConstants->GetResource()->GetGPUVirtualAddress());
        }

        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
    }

    // メイン描画設定
    ID3D12PipelineState* pso = env.psoManager->GetPSO(psoName);
    if (!pso)
    {
        pso = env.psoManager->GetPSO(isSkinning ? "Skinning" : "Standard3D");
    }
    cmdList->SetPipelineState(pso);
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature(isSkinning ? "Skinning" : "3D"));

    if (isSkinning)
    {
        const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
        D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
        cmdList->IASetVertexBuffers(0, 2, vbvs);
    }
    else
    {
        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
    }
    cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

    // 定数バッファ
    cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetPointLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetSpotLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetAreaLightResource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(5, sub.materialHandle.resource->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootConstantBufferView(6, buffer.wvpResource->GetGPUVirtualAddress());

    // テクスチャ
    cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(sub.textureHandle));
    cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(sub.envMapSrvHandle));
    cmdList->SetGraphicsRootDescriptorTable(9, shadowMap->GetSRVHandle());
    cmdList->SetGraphicsRootDescriptorTable(10, env.srvManager->GetSRVHandleGPU(sub.toonRampHandle));
    cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));
    cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(sub.normalMapHandle));
    cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(sub.rippleTextureHandle));
    cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(sub.puddleNoiseHandle));

    if (isSkinning)
    {
        cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
    }

    cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
}

void ModelRenderer::DrawShadow(const RenderEnvironment& env)
{
    auto* cmdList = env.commandManager->GetCommandList();

    // 通常モデル用の設定
    cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMap"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMap"));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());

    for (const auto& sub : modelSubmissions_)
    {
        if (sub.type != RenderType::Model && sub.type != RenderType::Skinning)
        {
            continue;
        }

        if (sub.group == RenderGroup::Background || sub.group == RenderGroup::UI)
        {
            continue;
        }

        if (sub.materialHandle.materialData->color.w <= 0.0f)
        {
            continue; // 透明度0なら影を描かない
        }

        // メッシュリストを取得して、正しいインデックスのMesh*を取り出す
        const std::vector<Mesh>& meshes = GetOrCreateModelBatch(*sub.modelData);
        assert(sub.meshIndex < meshes.size());
        const Mesh* mesh = &meshes[sub.meshIndex];

        auto& buffer = perObjectBuffers_[sub.instanceIndex];

        bool isSkinning = (sub.skinCluster != nullptr);

        bool needDissolve = (sub.materialHandle.materialData->enableDissolve != 0) ||
            (sub.materialHandle.materialData->color.w < 1.0f);

        // ディゾルブ・透明処理が必要な場合（重い処理）
        if (needDissolve)
        {
            if (isSkinning)
            {
                // スキニング・ディゾルブ影
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapSkinningDissolve"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapSkinningDissolve"));

                cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
                cmdList->SetGraphicsRootConstantBufferView(3, sub.materialHandle.resource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));

                const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
                D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
                cmdList->IASetVertexBuffers(0, 2, vbvs);
            }
            else
            {
                // 通常・ディゾルブ影
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapDissolve"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapDissolve"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, sub.materialHandle.resource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(2, buffer.wvpResource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));

                cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            }
        }
        // 不透明の場合（高速処理）
        else
        {
            if (isSkinning)
            {
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapSkinning"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapSkinning"));

                cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));

                const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
                D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(),  influence.influenceBufferView };
                cmdList->IASetVertexBuffers(0, 2, vbvs);
            }
            else
            {
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMap"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMap"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, sub.materialHandle.resource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(2, buffer.wvpResource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(3, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());

                cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            }
        }

        // 描画
        cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
        cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
    }
}