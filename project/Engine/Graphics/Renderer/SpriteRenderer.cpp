#include "pch.h"
#include "SpriteRenderer.h"
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

void SpriteRenderer::Initialize(const RenderEnvironment& env, int clientWidth, int clientHeight)
{
    clientWidth_ = clientWidth;
    clientHeight_ = clientHeight;

    // スプライトの配列を確保
    sprites_.resize(kMaxCount);

    // 左上原点のスプライト用頂点データ
    std::vector<VertexData> spriteVertices =
    {
        {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 左上
        {{1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 右上
        {{0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下
        {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 右下
    };

    // スプライト共通のインデックス
    std::vector<uint32_t> spriteIndices = { 0, 1, 2, 1, 3, 2 };

    // スプライト用メッシュとバッファを生成
    for (size_t i = 0; i < kMaxCount; ++i)
    {
        sprites_[i].mesh.Initialize(env.device->GetDevice(), spriteVertices, spriteIndices);

        sprites_[i].wvpResource = BufferManager::CreateBufferResource(env.device->GetDevice(), sizeof(TransformationMatrix));
        sprites_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&sprites_[i].mappedData));

        sprites_[i].mesh.SetIndexCount(spriteIndices.size());
    }

    index_ = 0;
    prevCount_ = 0;
}

void SpriteRenderer::BeginFrame()
{
    // フレーム開始時にカウンタとキューをリセット
    prevCount_ = index_;
    index_ = 0;
    submissions_.clear();
}

void SpriteRenderer::Submit(
    const Vector2& position, const Vector2& size, float rotation, uint32_t color,
    const Vector2& anchorPoint, const WorldTransform& uvTransform,
    uint32_t textureHandle, uint32_t dissolveTextureHandle, int layerOrder,
    const MaterialHandle& materialHandle)
{
    assert(index_ < kMaxCount);

    SpriteData& sprite = sprites_[index_];

    // マテリアル設定
    materialHandle.materialData->color = Math::Uint32ToColorVector(color);
    materialHandle.materialData->enableLighting = false;

    // UV変換行列設定
    Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
    uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
    materialHandle.materialData->uvTransform = uvTransformMatrix;

    // 行列計算
    Matrix4x4 anchorMatrix = Matrix4x4::MakeTranslate({ -anchorPoint.x, -anchorPoint.y, 0.0f });
    Matrix4x4 scaleMatrix = Matrix4x4::MakeScale({ size.x, size.y, 1.0f });
    Matrix4x4 rotationMatrix = Matrix4x4::MakeRotateZ(rotation);
    Matrix4x4 translateMatrix = Matrix4x4::MakeTranslate({ position.x, position.y, 0.0f });

    // 全て合成してワールド行列を作成
    Matrix4x4 worldMatrix = anchorMatrix * scaleMatrix * rotationMatrix * translateMatrix;

    // 平行投影行列を作成
    Matrix4x4 projectionMatrix = Matrix4x4::MakeOrthographic(
        0.0f, 0.0f, float(clientWidth_), float(clientHeight_),
        0.0f, 100.0f
    );

    Matrix4x4 wvpMatrix = worldMatrix * projectionMatrix;

    // 定数バッファにコピー
    sprite.mappedData->WVP = wvpMatrix;
    sprite.mappedData->World = worldMatrix;

    // SpriteRenderer自身の描画キューに登録
    SpriteSubmission submission{};
    submission.instanceIndex = index_;
    submission.textureHandle = textureHandle;
    submission.dissolveTextureHandle = dissolveTextureHandle;
    submission.materialHandle = materialHandle;
    submission.layerOrder = layerOrder;

    submissions_.push_back(submission);

    index_++;
}

void SpriteRenderer::Draw(const RenderEnvironment& env)
{
    // 描画するものがなければスキップ
    if (submissions_.empty()) return;

    // スプライトをレイヤーオーダー（描画順）でソート
    std::sort(submissions_.begin(), submissions_.end(),
        [](const SpriteSubmission& a, const SpriteSubmission& b)
        {
            if (a.layerOrder != b.layerOrder)
            {
                return a.layerOrder < b.layerOrder;
            }
            return a.instanceIndex < b.instanceIndex;
        });

    auto* cmdList = env.commandManager->GetCommandList();

    // スプライト描画パス全体のスコープ
    PIXScopedEvent(cmdList, FE::PIXColors::UI, "Sprite Pass (%zu Sprites)", submissions_.size());

    // 共通設定
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->SetPipelineState(env.psoManager->GetPSO("Sprite"));
    cmdList->SetGraphicsRootSignature(env.rootSignatureManager->GetRootSignature("Sprite"));

    // 登録されたスプライトを一気に描画
    for (size_t i = 0; i < submissions_.size(); ++i)
    {
        const auto& sub = submissions_[i];
        SpriteData& sprite = sprites_[sub.instanceIndex];

        // スプライトごとの個別スコープ 
        PIXScopedEvent(cmdList, FE::PIXColors::UI, "Sprite [%zu] (Layer: %d, Instance: %u)", i, sub.layerOrder, sub.instanceIndex);

        cmdList->IASetIndexBuffer(&sprite.mesh.GetIndexBufferView());
        cmdList->IASetVertexBuffers(0, 1, &sprite.mesh.GetVertexBufferView());

        cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, sprite.wvpResource->GetGPUVirtualAddress());
        cmdList->SetGraphicsRootDescriptorTable(2, env.srvManager->GetSRVHandleGPU(sub.textureHandle));
        cmdList->SetGraphicsRootDescriptorTable(3, env.srvManager->GetSRVHandleGPU(sub.dissolveTextureHandle));

        cmdList->DrawIndexedInstanced(uint32_t(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
    }
}

}