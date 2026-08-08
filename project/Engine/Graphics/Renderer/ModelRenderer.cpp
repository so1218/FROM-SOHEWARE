#include "pch.h"
#include "ModelRenderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "BufferManager.h"
#include "PostEffectManager.h"

namespace FE
{

void ModelRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device; 

    perObjectBuffers_.resize(kMaxCount);
    for (auto& buffer : perObjectBuffers_)
    {
        buffer.wvpResource = BufferManager::CreateMappedConstantBuffer(
            device_->GetDevice(),
            &buffer.wvpMapped
        );
    }

    // インスタンシング用バッファの初期化
    // StructuredBufferとして作成
    instanceBuffer_.resource = BufferManager::CreateMappedBuffer(
        device_->GetDevice(),
        kMaxInstances,
        &instanceBuffer_.mapped
    );

    // SRVの作成
    instanceBuffer_.srvIndex = env.srvManager->Allocate();
    instanceBuffer_.srvIndex = env.srvManager->CreateStructuredBufferSRV(
        instanceBuffer_.resource.Get(),
        kMaxInstances,
        sizeof(Object3DInstanceData)
    );
}

void ModelRenderer::Finalize()
{
    meshCache_.clear();
}

void ModelRenderer::BeginFrame()
{
    prevCount_ = indexModel_;
    indexModel_ = 0;
    modelSubmissions_.clear();
    currentInstanceLocation_ = 0;
    batches_.clear();
}

void ModelRenderer::SetCameraState(const Matrix4x4& view, const Matrix4x4& viewProjection)
{
    viewMatrix_ = view;
    viewProjectionMatrix_ = viewProjection;
}

const std::vector<Mesh>& ModelRenderer::GetOrCreateBatch(const ModelData& modelData)
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

void ModelRenderer::Submit(const WorldTransform& worldTransform, const ModelData& modelData,
    const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
    DepthMode depthMode, RenderGroup group, const Vector4& instanceColor,
    const Frustum& cameraFrustum, const std::vector<Frustum>& shadowFrustums)
{
    // モデルに対応するGPUメッシュリストを取得
    const auto& meshes = GetOrCreateBatch(modelData);

    // 再帰的にノードを巡回するラムダ関数
    std::function<void(const Node&, const Matrix4x4&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix, const Matrix4x4& parentPrevMatrix)
        {
            Matrix4x4 currentWorldMatrix = node.localMatrix * parentMatrix;
            Matrix4x4 currentPrevWorldMatrix = node.localMatrix * parentPrevMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                if (indexModel_ >= kMaxCount) return; // 安全対策

                const auto& meshPart = modelData.meshes[meshIndex];

                // カリング判定（AABBのワールド変換と交差判定）
                Vector3 localCenter = {
                    (meshPart.localAABB.min.x + meshPart.localAABB.max.x) * 0.5f,
                    (meshPart.localAABB.min.y + meshPart.localAABB.max.y) * 0.5f,
                    (meshPart.localAABB.min.z + meshPart.localAABB.max.z) * 0.5f
                };
                Vector3 localExtents = {
                    (meshPart.localAABB.max.x - meshPart.localAABB.min.x) * 0.5f,
                    (meshPart.localAABB.max.y - meshPart.localAABB.min.y) * 0.5f,
                    (meshPart.localAABB.max.z - meshPart.localAABB.min.z) * 0.5f
                };

                Vector3 worldCenter = currentWorldMatrix.TransformPoint(localCenter);

                // 行列のスケール・回転を適用した広がりを計算（絶対値）
                Vector3 worldExtents;
                worldExtents.x = std::abs(currentWorldMatrix.m[0][0]) * localExtents.x + std::abs(currentWorldMatrix.m[1][0]) * localExtents.y + std::abs(currentWorldMatrix.m[2][0]) * localExtents.z;
                worldExtents.y = std::abs(currentWorldMatrix.m[0][1]) * localExtents.x + std::abs(currentWorldMatrix.m[1][1]) * localExtents.y + std::abs(currentWorldMatrix.m[2][1]) * localExtents.z;
                worldExtents.z = std::abs(currentWorldMatrix.m[0][2]) * localExtents.x + std::abs(currentWorldMatrix.m[1][2]) * localExtents.y + std::abs(currentWorldMatrix.m[2][2]) * localExtents.z;

                Vector3 worldMin = { worldCenter.x - worldExtents.x, worldCenter.y - worldExtents.y, worldCenter.z - worldExtents.z };
                Vector3 worldMax = { worldCenter.x + worldExtents.x, worldCenter.y + worldExtents.y, worldCenter.z + worldExtents.z };

                // カメラ・影フラスタムとの判定
                bool isVisibleCamera = cameraFrustum.IntersectsAABB(worldMin, worldMax);
                bool isVisibleShadow = false;
                for (const auto& shadowFrustum : shadowFrustums)
                {
                    if (shadowFrustum.IntersectsAABB(worldMin, worldMax))
                    {
                        isVisibleShadow = true;
                        break;
                    }
                }

                // 両方から見えない場合は描画対象から除外（カリング）
                if (!isVisibleCamera && !isVisibleShadow)
                {
                    continue;
                }

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
                buffer.wvpMapped->PrevWorld = currentPrevWorldMatrix;

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
                submission.heightMapHandle = actualMaterialHandle.heightMapHandle;
                submission.rippleTextureHandle = actualMaterialHandle.rippleTextureHandle;
                submission.puddleNoiseHandle = actualMaterialHandle.puddleNoiseHandle;
                submission.worldMatrix = currentWorldMatrix;
                submission.worldInverseTranspose = Matrix4x4::Inverse(currentWorldMatrix.Transpose());
                submission.instancingColor = instanceColor;
                submission.wvpMatrix = wvp;
                submission.prevWorldMatrix = currentPrevWorldMatrix;

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
                submission.isVisibleCamera = isVisibleCamera;
                submission.isVisibleShadow = isVisibleShadow;

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
                Traverse(child, currentWorldMatrix, currentPrevWorldMatrix);
            }
        };

        Traverse(modelData.rootNode, worldTransform.matWorld_, worldTransform.matWorldPrev_);
}

void ModelRenderer::SubmitAnimation(
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
    GetOrCreateBatch(*modelData);

    for (size_t i = 0; i < modelData->meshes.size(); ++i)
    {
        assert(indexModel_ < kMaxCount);

        const auto& meshPart = modelData->meshes[i];
        auto& buffer = perObjectBuffers_[indexModel_];

        // 各パーツのWorld行列はモデル全体のWorldで統一
        Matrix4x4 world = worldTransform.matWorld_;
        Matrix4x4 prevWorld = worldTransform.matWorldPrev_;
        Matrix4x4 wvp = world * viewProjectionMatrix_;
        buffer.wvpMapped->WVP = wvp;
        buffer.wvpMapped->World = world;
        buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
        buffer.wvpMapped->WorldColor = instanceColor;
        buffer.wvpMapped->PrevWorld = prevWorld;

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
        submission.heightMapHandle = actualMaterialHandle.heightMapHandle;
        submission.rippleTextureHandle = actualMaterialHandle.rippleTextureHandle;
        submission.puddleNoiseHandle = actualMaterialHandle.puddleNoiseHandle;
        submission.worldMatrix = world;
        submission.wvpMatrix = wvp;
        submission.worldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
        submission.instancingColor = instanceColor;
        submission.prevWorldMatrix = prevWorld;

        // マテリアルデータのポインタが存在し、かつenableOutlineがtrueなら有効
        if (actualMaterialHandle.materialData)
        {
            submission.enableOutline = (actualMaterialHandle.materialData->enableOutline != 0);
        }
        else 
        {
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
    if (batches_.empty()) return;

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // バッチ単位で回す
    for (const auto& batch : batches_)
    {
        const auto& sub = *batch.baseSubmission;

        // 指定のグループじゃなければスキップ
        if (sub.group != targetGroup) continue;

        // バッチの情報を渡して描画コアを呼ぶ
        DrawCore(env, sub, isWireFrame, shadowMap, batch.instanceCount, batch.startInstanceLocation);
    }
}

void ModelRenderer::DrawCore(const RenderEnvironment& env, const ModelSubmission& sub, bool isWireFrame, ShadowMap* shadowMap,
    uint32_t instanceCount, uint32_t startInstanceLocation)
{
    const std::vector<Mesh>& meshes = GetOrCreateBatch(*sub.modelData);
    assert(sub.meshIndex < meshes.size());
    const Mesh* mesh = &meshes[sub.meshIndex];

    auto* cmdList = env.commandManager->GetCommandList();
    uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

    bool isSkinning = (sub.skinCluster != nullptr);

    std::string psoName;
    if (isSkinning)
    {
        psoName = "Skinning";
    }
    else
    {
        switch (sub.blendMode)
        {
        case BlendMode::kBlendModeAdd:      psoName = "Object3D_Add";         break;
        case BlendMode::kBlendModeNormal:   psoName = "Object3D_Transparent"; break;
        case BlendMode::kBlendModeNone:
        default:                            psoName = "Object3D_Opaque";          break;
        }
    }

    if (isWireFrame)
    {
        psoName += "_Wireframe";
    }
    else
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
            auto& buffer = perObjectBuffers_[sub.instanceIndex]; 
            cmdList->SetPipelineState(env.psoManager->GetPSO("OutlineSkinning"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("OutlineSkinning"));

            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootDescriptorTable(1, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
            cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
            cmdList->SetGraphicsRootConstantBufferView(3, buffer.wvpResource->GetGPUVirtualAddress());

            const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
            D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
            cmdList->IASetVertexBuffers(0, 2, vbvs);

            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
            cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }
        else
        {
            // 静的モデルをインスタンシング描画
            cmdList->SetPipelineState(env.psoManager->GetPSO("OutlineInstanced"));
            cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Instancing3D"));

            cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

            // インスタンシング用の共通定数とリソースをセット
            cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); 
            cmdList->SetGraphicsRootConstantBufferView(5, sub.materialHandle.resource->GetGPUVirtualAddress());       
            cmdList->SetGraphicsRoot32BitConstant(6, startInstanceLocation, 0);                                      
            cmdList->SetGraphicsRootDescriptorTable(17, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));

            cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, startInstanceLocation);
        }
    }

    if (isSkinning)
    {
        // スキニング用設定
        ID3D12PipelineState* pso = env.psoManager->GetPSO(psoName);
        if (!pso) { pso = env.psoManager->GetPSO("Skinning"); }
        cmdList->SetPipelineState(pso);
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skinning"));

        const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
        D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
        cmdList->IASetVertexBuffers(0, 2, vbvs);
    }
    else
    {
        ID3D12PipelineState* pso = env.psoManager->GetPSO(psoName);
        if (!pso) { pso = env.psoManager->GetPSO("Object3D_Opaque"); } 
        cmdList->SetPipelineState(pso);
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Instancing3D"));

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
    cmdList->SetGraphicsRootConstantBufferView(7, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());

    // テクスチャ
    cmdList->SetGraphicsRootDescriptorTable(8, env.srvManager->GetSRVHandleGPU(sub.textureHandle));
    cmdList->SetGraphicsRootDescriptorTable(9, env.srvManager->GetSRVHandleGPU(sub.envMapSrvHandle));
    cmdList->SetGraphicsRootDescriptorTable(10, shadowMap->GetSRVHandle());
    cmdList->SetGraphicsRootDescriptorTable(11, env.srvManager->GetSRVHandleGPU(sub.toonRampHandle));
    cmdList->SetGraphicsRootDescriptorTable(12, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));
    cmdList->SetGraphicsRootDescriptorTable(13, env.srvManager->GetSRVHandleGPU(sub.normalMapHandle));
    cmdList->SetGraphicsRootDescriptorTable(14, env.srvManager->GetSRVHandleGPU(sub.rippleTextureHandle));
    cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(sub.puddleNoiseHandle));
    cmdList->SetGraphicsRootDescriptorTable(16, env.srvManager->GetSRVHandleGPU(sub.heightMapHandle));

    if (isSkinning)
    {
        auto& buffer = perObjectBuffers_[sub.instanceIndex]; // スキニングは個別バッファ
        cmdList->SetGraphicsRootConstantBufferView(6, buffer.wvpResource->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(17, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
        cmdList->SetGraphicsRootDescriptorTable(18, env.srvManager->GetSRVHandleGPU(sub.skinCluster->prevPaletteSrvIndex));
        cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
    else
    {
        // 静的モデルはインスタンシング描画
        cmdList->SetGraphicsRoot32BitConstant(6, startInstanceLocation, 0);
        cmdList->SetGraphicsRootDescriptorTable(17, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
        cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
    }
}

void ModelRenderer::DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex)
{
    auto* cmdList = env.commandManager->GetCommandList();
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 作成済みのbatches_を回す
    for (const auto& batch : batches_)
    {
        // バッチの基準となるデータを取得
        const auto& sub = *batch.baseSubmission;

        // フィルタリング処理
        if (sub.type != RenderType::Model && sub.type != RenderType::Skinning) continue;
        if (sub.group == RenderGroup::Background || sub.group == RenderGroup::UI) continue;
        if (sub.materialHandle.materialData->color.w <= 0.0f) continue; // 透明度0なら影を描かない

        const std::vector<Mesh>& meshes = GetOrCreateBatch(*sub.modelData);
        assert(sub.meshIndex < meshes.size());
        const Mesh* mesh = &meshes[sub.meshIndex];

        auto& buffer = perObjectBuffers_[sub.instanceIndex];
        bool isSkinning = (sub.skinCluster != nullptr);
        bool needDissolve = (sub.materialHandle.materialData->enableDissolve != 0) ||
            (sub.materialHandle.materialData->color.w < 1.0f);

        // ディゾルブ
        if (needDissolve)
        {
            if (isSkinning)
            {
                // スキニング・ディゾルブ影
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapSkinningDissolve"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapSkinningDissolve"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());

                cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstant(3, cascadeIndex, 0);

                cmdList->SetGraphicsRootConstantBufferView(4, sub.materialHandle.resource->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootDescriptorTable(5, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
                cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));

                const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
                D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
                cmdList->IASetVertexBuffers(0, 2, vbvs);
                cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
                cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
            }
            else
            {
                // 静的モデル・ディゾルブ影 
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapDissolveInstanced"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapDissolveInstanced"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRoot32BitConstant(3, batch.startInstanceLocation, 0); 
                cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRoot32BitConstant(5, cascadeIndex, 0);
                cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex)); 

                // ディゾルブテクスチャ
                cmdList->SetGraphicsRootDescriptorTable(7, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle)); 

                cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
                cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
                cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), batch.instanceCount, 0, 0, 0);
            }
        }
        else
        {
            if (isSkinning)
            {
                // スキニング・通常影
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapSkinning"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapSkinning"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());

                cmdList->SetGraphicsRootConstantBufferView(2, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress());
                cmdList->SetGraphicsRoot32BitConstant(3, cascadeIndex, 0);

                cmdList->SetGraphicsRootDescriptorTable(4, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));

                const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
                D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(),  influence.influenceBufferView };
                cmdList->IASetVertexBuffers(0, 2, vbvs);
                cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
                cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
            }
            else
            {
                // 静的モデル・通常影
                cmdList->SetPipelineState(env.psoManager->GetPSO("ShadowMapInstanced"));
                cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("ShadowMapInstanced"));

                cmdList->SetGraphicsRootConstantBufferView(0, env.globalConstants->GetResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRootConstantBufferView(1, env.lightManager->GetDirectionalLightResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRoot32BitConstant(3, batch.startInstanceLocation, 0); 
                cmdList->SetGraphicsRootConstantBufferView(4, env.lightManager->GetShadowDataResource()->GetGPUVirtualAddress()); 
                cmdList->SetGraphicsRoot32BitConstant(5, cascadeIndex, 0); 
                cmdList->SetGraphicsRootDescriptorTable(6, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex)); 

                cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
                cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
                cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), batch.instanceCount, 0, 0, 0);
            }
        }
    }
}

void ModelRenderer::PrepareBatches()
{
    batches_.clear();
    if (modelSubmissions_.empty()) return;

    // 全Submissionをソート
    std::sort(modelSubmissions_.begin(), modelSubmissions_.end(),
        [](const ModelSubmission& a, const ModelSubmission& b) {
            if (a.group != b.group) return a.group < b.group;
            if (a.group == RenderGroup::Transparent) return a.depth > b.depth;
            if (a.type != b.type) return a.type < b.type;
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;
            return a.materialHandle.materialData < b.materialHandle.materialData;
        });

    // バッチの作成とインスタンスバッファの構築
    uint32_t instanceCount = 0;

    for (size_t i = 0; i < modelSubmissions_.size(); ++i)
    {
        const auto& sub = modelSubmissions_[i];

        // インスタンスデータをバッファに書き込む
        auto& instanceData = instanceBuffer_.mapped[currentInstanceLocation_ + instanceCount];
        instanceData.World = sub.worldMatrix;
        instanceData.WorldInverseTranspose = sub.worldInverseTranspose;
        instanceData.WorldColor = sub.instancingColor;
        instanceData.PrevWorld = sub.prevWorldMatrix;

        instanceCount++;

        bool isLast = (i == modelSubmissions_.size() - 1);
        bool shouldFlush = isLast;

        if (!isLast)
        {
            const auto& nextSub = modelSubmissions_[i + 1];
            // 描画条件が変わったらバッチを区切る
            if (sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.materialHandle.materialData != nextSub.materialHandle.materialData ||
                sub.type != nextSub.type ||
                sub.type == RenderType::Skinning ||
                sub.group != nextSub.group) // グループが変わっても区切る
            {
                shouldFlush = true;
            }
        }

        if (shouldFlush)
        {
            // バッチを登録
            RenderBatch batch;
            batch.baseSubmission = &sub;
            batch.instanceCount = instanceCount;
            batch.startInstanceLocation = currentInstanceLocation_;
            batches_.push_back(batch);

            currentInstanceLocation_ += instanceCount;
            instanceCount = 0;

            if (currentInstanceLocation_ >= kMaxInstances) break;
        }
    }
}

}