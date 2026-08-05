#include "pch.h"
#include "Model.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Engine.h"

namespace FE
{

Model::Model(Engine* engine, const std::string& modelName)
    : Model(engine, ModelManager::GetInstance().Get(modelName))
{
}

Model::Model(Engine* engine, const ModelData* modelData)
    : engine_(engine), modelData_(modelData)
{
    if (!modelData_ || !engine_) return;

    materials_ = modelData_->defaultMaterials;
    isMaterialsUnique_ = false;
}

void Model::Draw()
{
    if (!modelData_ || !engine_) return;

    // CPUでの行列更新
    transform_.UpdateMatrix();

    // 描画命令発行
    engine_->GetRendererManager()->SubmitModel(
        transform_,
        *modelData_,
        materials_,
        blendMode_,
        cullMode_,
        depthMode_,
        renderGroup_,
        baseColor_
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
    MakeMaterialUnique();

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

void Model::SetTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    // 文字列からGPUハンドルを検索して取得
    uint32_t handle = TextureManager::GetInstance().Get(textureName);

    // 全マテリアルに適用
    for (auto& mat : materials_) mat.textureHandle = handle;
}

void Model::SetEnvironmentMapTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.envMapHandle = handle;
}

void Model::SetToonRampTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.toonRampHandle = handle;
}

void Model::SetDissolveTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.dissolveMapHandle = handle;
}

void Model::SetNormalMapTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.normalMapHandle = handle;
}

void Model::SetRippleTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.rippleTextureHandle = handle;
}

void Model::SetPuddleNoiseTexture(const std::string& textureName)
{
    MakeMaterialUnique();

    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.puddleNoiseHandle = handle;
}

void Model::CopyMaterialsFrom(const Model* sourceModel)
{
    if (!sourceModel) return;

    // マテリアル数が違う場合はコピーしない（安全対策）
    size_t count = GetMaterialCount();
    if (count != sourceModel->GetMaterialCount()) return;

    MakeMaterialUnique();

    // ベースカラーのコピー
    SetBaseColor(sourceModel->GetBaseColor());

    for (size_t i = 0; i < count; ++i)
    {
        MaterialData* myData = GetMaterialData(i);
        const MaterialData* sourceData = sourceModel->GetMaterialData(i);

        if (myData && sourceData)
        {
            // 構造体のコピー
            *myData = *sourceData;
        }

        // テクスチャやハンドルの名前も同期する
        MaterialHandle* myHandle = GetMaterialHandle(i);
        MaterialHandle* sourceHandle = const_cast<Model*>(sourceModel)->GetMaterialHandle(i);

        if (myHandle && sourceHandle)
        {
            myHandle->textureName = sourceHandle->textureName;
            myHandle->textureHandle = sourceHandle->textureHandle;
            myHandle->normalMapName = sourceHandle->normalMapName;
            myHandle->normalMapHandle = sourceHandle->normalMapHandle;
            myHandle->envMapName = sourceHandle->envMapName;
            myHandle->envMapHandle = sourceHandle->envMapHandle;
            myHandle->dissolveMapName = sourceHandle->dissolveMapName;
            myHandle->dissolveMapHandle = sourceHandle->dissolveMapHandle;
            myHandle->toonRampName = sourceHandle->toonRampName;
            myHandle->toonRampHandle = sourceHandle->toonRampHandle;
            myHandle->puddleNoiseName = sourceHandle->puddleNoiseName;
            myHandle->puddleNoiseHandle = sourceHandle->puddleNoiseHandle;
            myHandle->rippleTextureName = sourceHandle->rippleTextureName;
            myHandle->rippleTextureHandle = sourceHandle->rippleTextureHandle;
        }
    }
}

void Model::ShareMaterialsFrom(const Model* sourceModel)
{
    if (!sourceModel) return;

    // そのままコピーすることで、同じ MaterialData を参照
    this->materials_ = sourceModel->materials_;

    this->isMaterialsUnique_ = false;
}

void Model::ShareModelDataFrom(const Model* sourceModel)
{
    if (!sourceModel) return;

    // 頂点データなどの実体ポインタをコピーして、同じアドレスを参照
    this->modelData_ = sourceModel->modelData_;
}

void Model::MakeMaterialUnique()
{
    if (isMaterialsUnique_) return;

    std::vector<MaterialHandle> newMaterials;
    newMaterials.reserve(materials_.size());

    for (const auto& oldMat : materials_)
    {
        MaterialHandle newMat = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

        // ハンドルと同時に、文字列（名前）も忘れずにコピーする
        newMat.textureName = oldMat.textureName;
        newMat.textureHandle = oldMat.textureHandle;
        newMat.envMapName = oldMat.envMapName;
        newMat.envMapHandle = oldMat.envMapHandle;
        newMat.toonRampName = oldMat.toonRampName;
        newMat.toonRampHandle = oldMat.toonRampHandle;
        newMat.dissolveMapName = oldMat.dissolveMapName;
        newMat.dissolveMapHandle = oldMat.dissolveMapHandle;
        newMat.normalMapName = oldMat.normalMapName;
        newMat.normalMapHandle = oldMat.normalMapHandle;
        newMat.heightMapName = oldMat.heightMapName;
        newMat.heightMapHandle = oldMat.heightMapHandle;
        newMat.rippleTextureName = oldMat.rippleTextureName;
        newMat.rippleTextureHandle = oldMat.rippleTextureHandle;
        newMat.puddleNoiseName = oldMat.puddleNoiseName;
        newMat.puddleNoiseHandle = oldMat.puddleNoiseHandle;

        // UVトランスフォームの設定値を引き継ぐ
        newMat.uvTransformData = oldMat.uvTransformData;

        // GPUに送る定数バッファの中身をコピー
        if (newMat.materialData && oldMat.materialData)
        {
            *(newMat.materialData) = *(oldMat.materialData);
        }

        newMaterials.push_back(newMat);
    }

    materials_ = newMaterials;
    isMaterialsUnique_ = true;
}

void Model::SetColor(const Vector4& color)
{
    MakeMaterialUnique();

    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->color = color;
    }
}

void Model::SetColor(uint32_t color)
{
    MakeMaterialUnique();

    SetColor(Math::Uint32ToColorVector(color));
}

void Model::SetBaseColor(uint32_t color) { baseColor_ = Math::Uint32ToColorVector(color); }

void Model::SetEmissiveIntensity(float intensity)
{
    MakeMaterialUnique();

    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->emissiveIntensity = intensity;
    }
}

void Model::SetEnableOutline(bool enable)
{
    MakeMaterialUnique();

    int flag = enable ? 1 : 0;
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->enableOutline = flag;
    }
}

void Model::SetOutlineWidth(float width)
{
    MakeMaterialUnique();

    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->outlineWidth = width;
    }
}

void Model::SetOutlineColor(const Vector4& color)
{
    MakeMaterialUnique();

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
    MakeMaterialUnique();

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
    MakeMaterialUnique();

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


void Model::ApplyRenderSettings(const RenderSettings& settings)
{
    SetBlendMode(settings.blendMode);
    SetCullMode(settings.cullMode);
    SetDepthMode(settings.depthMode);
    SetRenderGroup(settings.renderGroup);
}

}