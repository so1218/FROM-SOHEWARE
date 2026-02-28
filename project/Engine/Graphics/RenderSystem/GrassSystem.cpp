#include "GrassSystem.h"
#include "Engine.h"
#include "ModelManager.h"

GrassSystem::GrassSystem(Engine* engine, const std::string& modelName, const std::string& textureName)
    : engine_(engine)
{
    const ModelData* modelData = ModelManager::GetInstance().Get(modelName);
    if (modelData) {
        engine_->GetRendererManager()->InitializeGrass(*modelData);
    }

    // テクスチャの設定
    SetTexture(textureName);

    // マテリアルの初期値設定
    materialData_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_.grassWindSpeed = 1.0f;
    materialData_.grassWindAmplitude = 0.5f;
    materialData_.grassNormalBlend = 0.5f;
    materialData_.grassTranslucency = 0.5f;
    materialData_.grassRootAO = 0.5f;
    materialData_.grassAlphaCutoff = 0.1f;
    materialData_.addShadow = 1;
}

void GrassSystem::AddGrass(const Vector3& position, const Vector3& rotation, const Vector3& scale, const Vector4& color)
{
    WorldTransform tempTransform;
    tempTransform.translation_ = position;
    tempTransform.rotation_ = rotation;
    tempTransform.scale_ = scale;
    tempTransform.UpdateMatrix();

    Instance inst;
    inst.worldMatrix = tempTransform.matWorld_;
    inst.color = color;

    instances_.push_back(inst);
}

void GrassSystem::AddGrass(const WorldTransform& transform, const Vector4& color)
{
    WorldTransform tempTransform = transform;
    tempTransform.UpdateMatrix();

    Instance inst;
    inst.worldMatrix = tempTransform.matWorld_; 
    inst.color = color;

    instances_.push_back(inst);
}

void GrassSystem::Clear()
{
    instances_.clear();
}

void GrassSystem::Draw()
{
    if (instances_.empty() || !engine_) return;

    auto* rendererManager = engine_->GetRendererManager();
    rendererManager->SetGrassRenderingParams(textureHandle_, materialData_);

    for (const auto& inst : instances_)
    {
        rendererManager->SubmitGrass(inst.worldMatrix, inst.color);
    }
}

void GrassSystem::SetTexture(const std::string& textureName)
{
    textureHandle_ = TextureManager::GetInstance().Get(textureName);
}

void GrassSystem::SetColor(const Vector4& color) { materialData_.color = color; }
void GrassSystem::SetWindSpeed(float speed) { materialData_.grassWindSpeed = speed; }
void GrassSystem::SetWindAmplitude(float amplitude) { materialData_.grassWindAmplitude = amplitude; }
void GrassSystem::SetNormalBlend(float blend) { materialData_.grassNormalBlend = blend; }
void GrassSystem::SetTranslucency(float translucency) { materialData_.grassTranslucency = translucency; }
void GrassSystem::SetRootAO(float ao) { materialData_.grassRootAO = ao; }
void GrassSystem::SetAlphaCutoff(float cutoff) { materialData_.grassAlphaCutoff = cutoff; }
void GrassSystem::SetEnableShadow(bool enable) { materialData_.addShadow = enable ? 1 : 0; }