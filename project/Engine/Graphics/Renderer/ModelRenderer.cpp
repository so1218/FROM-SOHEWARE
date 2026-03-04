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

void ModelRenderer::Initialize(const RenderEnvironment& env)
{
    device_ = env.device; 

    perObjectBuffers_.resize(kMaxCount);
    for (auto& buffer : perObjectBuffers_)
    {
        buffer.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
        buffer.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.wvpMapped));
    }

    // インスタンシング用バッファの初期化
    // StructuredBufferとして作成
    instanceBuffer_.resource = BufferManager::CreateBufferResource(
        device_->GetDevice(), sizeof(Object3DInstanceData) * kMaxInstances);
    instanceBuffer_.resource->Map(0, nullptr, reinterpret_cast<void**>(&instanceBuffer_.mapped));

    // SRVの作成
    instanceBuffer_.srvIndex = env.srvManager->Allocate();
    env.srvManager->CreateStructuredBufferSRV(
        instanceBuffer_.srvIndex, instanceBuffer_.resource.Get(), kMaxInstances, sizeof(Object3DInstanceData));
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
    DepthMode depthMode, RenderGroup group, const Vector4& instanceColor)
{
    // モデルに対応するGPUメッシュリストを取得
    const auto& meshes = GetOrCreateBatch(modelData);

    // 再帰的にノードを巡回するラムダ関数
    std::function<void(const Node&, const Matrix4x4&)> Traverse =
        [&](const Node& node, const Matrix4x4& parentMatrix)
        {
            Matrix4x4 currentWorldMatrix = node.localMatrix * parentMatrix;

            for (unsigned int meshIndex : node.meshIndices)
            {
                if (indexModel_ >= kMaxCount) return; // 安全対策

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
                submission.worldInverseTranspose = Matrix4x4::Inverse(currentWorldMatrix.Transpose());
                submission.instancingColor = instanceColor;
                submission.wvpMatrix = wvp;

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
        submission.wvpMatrix = wvp;
        submission.worldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
        submission.instancingColor = instanceColor;

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
    if (modelSubmissions_.empty()) return;

    std::sort(modelSubmissions_.begin(), modelSubmissions_.end(),
        [](const ModelSubmission& a, const ModelSubmission& b)
        {
            // グループ順
            if (a.group != b.group) return a.group < b.group;

            // 半透明は奥から手前へ
            if (a.group == RenderGroup::Transparent)
            {
                return a.depth > b.depth;
            }

            // スキニングかどうか
            if (a.type != b.type) return a.type < b.type;

            // 同じモデル・メッシュ・マテリアルをまとめる 
            if (a.modelData != b.modelData) return a.modelData < b.modelData;
            if (a.meshIndex != b.meshIndex) return a.meshIndex < b.meshIndex;

            // 最後にマテリアル
            return a.materialHandle.materialData < b.materialHandle.materialData;
        });

    auto* cmdList = env.commandManager->GetCommandList();
    ID3D12DescriptorHeap* heaps[] = { env.srvManager->GetSRVHeap() };
    cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    uint32_t instanceCount = 0;

    for (size_t i = 0; i < modelSubmissions_.size(); ++i)
    {
        const auto& sub = modelSubmissions_[i];
        if (sub.group != targetGroup) continue;

        // インスタンスデータをバッファに書き込む
        auto& instanceData = instanceBuffer_.mapped[currentInstanceLocation_ + instanceCount];
        instanceData.World = sub.worldMatrix;
        instanceData.WorldInverseTranspose = sub.worldInverseTranspose;
        instanceData.WorldColor = sub.instancingColor;

        instanceCount++;

        // バッチを区切って描画するか判定
        bool isLast = (i == modelSubmissions_.size() - 1);
        bool shouldFlush = isLast;

        if (!isLast)
        {
            const auto& nextSub = modelSubmissions_[i + 1];
            // 次のSubmissionとモデル・メッシュ・マテリアル・描画タイプが違うなら区切る
            if (sub.modelData != nextSub.modelData ||
                sub.meshIndex != nextSub.meshIndex ||
                sub.materialHandle.materialData != nextSub.materialHandle.materialData ||
                sub.type != nextSub.type ||
                sub.type == RenderType::Skinning) // スキニングは現状1つずつ描画
            {
                shouldFlush = true;
            }
        }

        // 溜まった分を一気に描画
        if (shouldFlush)
        {
            // DrawCoreに描画するインスタンス数と開始位置を渡す
            DrawCore(env, sub, isWireFrame, shadowMap, instanceCount, currentInstanceLocation_);

            // 次のバッチのためにリセット
            currentInstanceLocation_ += instanceCount;
            instanceCount = 0;

            // 安全対策:バッファの最大数を超えないように
            if (currentInstanceLocation_ >= kMaxInstances) {
                break;
            }
        }
    }
}

void ModelRenderer::DrawCore(const RenderEnvironment& env, const ModelSubmission& sub, bool isWireFrame, ShadowMap* shadowMap,
    uint32_t instanceCount, uint32_t startInstanceLocation)
{
    const std::vector<Mesh>& meshes = GetOrCreateBatch(*sub.modelData);
    assert(sub.meshIndex < meshes.size());
    const Mesh* mesh = &meshes[sub.meshIndex];

    auto& buffer = perObjectBuffers_[sub.instanceIndex];
    auto* cmdList = env.commandManager->GetCommandList();
    uint32_t indexCount = static_cast<uint32_t>(mesh->GetIndexCount());

    // 3つの状態を定義
    bool isSkinning = (sub.skinCluster != nullptr);
    bool isInstancing = (!isSkinning && instanceCount > 1); // 2個以上ならインスタンシング
    bool isStandard = (!isSkinning && instanceCount == 1);  // 1個なら通常の描画

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
        else if (isInstancing)
        {
            // インスタンシング用のアウトライン
            // cmdList->SetPipelineState(env.psoManager->GetPSO("InstancingOutline"));
            // cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("InstancingOutline"));
            // cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
            // cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
            // cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), instanceCount, 0, 0, startInstanceLocation);
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
    if (!pso) { pso = env.psoManager->GetPSO(isSkinning ? "Skinning" : "Standard3D"); }

    if (isSkinning)
    {
        cmdList->SetPipelineState(pso);
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Skinning"));

        const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
        D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
        cmdList->IASetVertexBuffers(0, 2, vbvs);
    }
    else if (isInstancing)
    {
        std::string instancingPsoName = "Instancing" + psoName;
        ID3D12PipelineState* instancingPso = env.psoManager->GetPSO(instancingPsoName);
        cmdList->SetPipelineState(instancingPso ? instancingPso : env.psoManager->GetPSO("InstancingStandard3D"));
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Instancing3D"));

        cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
    }
    else 
    {
        cmdList->SetPipelineState(pso);
        cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("3D"));

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
        // スキニング用
        cmdList->SetGraphicsRootConstantBufferView(6, buffer.wvpResource->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
        cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
    else if (isInstancing)
    {
        // インスタンシング用
        cmdList->SetGraphicsRoot32BitConstant(6, startInstanceLocation, 0);
        cmdList->SetGraphicsRootDescriptorTable(15, env.srvManager->GetSRVHandleGPU(instanceBuffer_.srvIndex));
        cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, startInstanceLocation);
    }
    else 
    {
        // 通常のモデル用
        cmdList->SetGraphicsRootConstantBufferView(6, buffer.wvpResource->GetGPUVirtualAddress());
        cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
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
        const std::vector<Mesh>& meshes = GetOrCreateBatch(*sub.modelData);
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