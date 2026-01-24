#include "Model.h"
#include "Engine.h"
#include "MaterialManager.h"

Model::Model(Engine* engine, const ModelData* modelData)
    : engine_(engine), modelData_(modelData)
{
    if (!modelData_ || !engine_) return;

    // メッシュの数だけマテリアルを確保する
    materials_.reserve(modelData_->meshes.size());

    for (const auto& mesh : modelData_->meshes)
    {
        // マテリアル作成
        MaterialHandle newMaterial = engine_->materialManager_->CreateMaterial(engine_->graphicsDevice_->GetDevice());

        // デフォルトテクスチャ設定
        newMaterial.textureHandle = TextureHandle::Get(TextureID::white1x1);
        newMaterial.envMapHandle = TextureHandle::Get(TextureID::skyboxCubemap);
        newMaterial.toonRampHandle = TextureHandle::Get(TextureID::toonRamp);
        newMaterial.dissolveMapHandle = TextureHandle::Get(TextureID::white1x1);
        newMaterial.normalMapHandle = TextureHandle::Get(TextureID::white1x1);

        // UVトランスフォーム初期化
        newMaterial.uvTransformData.Initialize();

        // 初期値をGPUバッファへ反映
        if (newMaterial.materialData)
        {
            newMaterial.materialData->uvTransform = newMaterial.uvTransformData.matWorld_;
        }

        materials_.push_back(newMaterial);
    }
}

void Model::Draw()
{
    if (!modelData_ || !engine_) return;

    // CPUでの行列更新
    transform_.UpdateMatrix();

    // 描画命令発行
    engine_->renderer_->SubmitModel(
        transform_,
        *modelData_,
        materials_,
        blendMode_,
        renderGroup_
    );
}

void Model::UpdateUV()
{
    for (auto& mat : materials_)
    {
        // 行列計算
        mat.uvTransformData.UpdateMatrix();

        // 定数バッファ転送
        if (mat.materialData)
        {
            mat.materialData->uvTransform = mat.uvTransformData.matWorld_;
        }
    }
}

// ========================================================================
// 一括設定
// ========================================================================

void Model::SetUVTransform(const WorldTransform& uvTransform)
{
    for (auto& mat : materials_)
    {
        mat.uvTransformData.translation_ = uvTransform.translation_;
        mat.uvTransformData.rotation_ = uvTransform.rotation_;
        mat.uvTransformData.scale_ = uvTransform.scale_;

        // 即座に行列更新とGPU転送を行う
        mat.uvTransformData.UpdateMatrix();
        if (mat.materialData) {
            mat.materialData->uvTransform = mat.uvTransformData.matWorld_;
        }
    }
}

void Model::SetTexture(TextureID textureID)
{
    uint32_t handle = TextureHandle::Get(textureID);
    for (auto& mat : materials_) mat.textureHandle = handle;
}

void Model::SetEnvironmentMapTexture(TextureID textureID)
{
    uint32_t handle = TextureHandle::Get(textureID);
    for (auto& mat : materials_) mat.envMapHandle = handle;
}

void Model::SetToonRampTexture(TextureID textureID)
{
    uint32_t handle = TextureHandle::Get(textureID);
    for (auto& mat : materials_) mat.toonRampHandle = handle;
}

void Model::SetDissolveTexture(TextureID textureID)
{
    uint32_t handle = TextureHandle::Get(textureID);
    for (auto& mat : materials_) mat.dissolveMapHandle = handle;
}

void Model::SetNormalMapTexture(TextureID textureID)
{
    uint32_t handle = TextureHandle::Get(textureID);
    for (auto& mat : materials_) mat.normalMapHandle = handle;
}

void Model::SetColor(const Vector4& color)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->color = color;
    }
}

void Model::SetColor(uint32_t color)
{
    SetColor(Math::Uint32ToColorVector(color));
}

void Model::SetEmissiveIntensity(float intensity)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->emissiveIntensity = intensity;
    }
}

void Model::SetEnableOutline(bool enable)
{
    int flag = enable ? 1 : 0;
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->enableOutline = flag;
    }
}

void Model::SetOutlineWidth(float width)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->outlineWidth = width;
    }
}

void Model::SetOutlineColor(const Vector4& color)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->outlineColor = color;
    }
}

void Model::SetOutlineColor(uint32_t color)
{
    SetOutlineColor(Math::Uint32ToColorVector(color));
}

void Model::SetEnableDissolve(bool enable)
{
    int flag = enable ? 1 : 0; // bool -> int/uint変換を明示
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->enableDissolve = flag;
    }
}

// ========================================================================
// 個別設定
// ========================================================================

void Model::SetMaterialColor(size_t index, const Vector4& color)
{
    if (IsValidMaterialIndex(index) && materials_[index].materialData) {
        materials_[index].materialData->color = color;
    }
}

void Model::SetMaterialColor(size_t index, uint32_t color)
{
    SetMaterialColor(index, Math::Uint32ToColorVector(color));
}

// ========================================================================
// ゲッター / アクセサ
// ========================================================================

WorldTransform* Model::GetUVTransform(size_t index)
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return &materials_[index].uvTransformData;
}

MaterialData* Model::GetMaterialData(size_t index)
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return materials_[index].materialData;
}

const MaterialData* Model::GetMaterialData(size_t index) const
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return materials_[index].materialData;
}

MaterialHandle* Model::GetMaterialHandle(size_t index)
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return &materials_[index];
}

Vector4* Model::GetMaterialColorPtr(size_t index)
{
    if (IsValidMaterialIndex(index) && materials_[index].materialData) {
        return &materials_[index].materialData->color;
    }
    return nullptr;
}

// ========================================================================
// プライベート
// ========================================================================

bool Model::IsValidMaterialIndex(size_t index) const
{
    return index < materials_.size();
}