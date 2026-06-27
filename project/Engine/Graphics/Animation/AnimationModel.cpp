#include "pch.h"
#include "AnimationModel.h"
#include "TimeManager.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "TextureManager.h"
#include "Engine.h"
#include "SRVManager.h"

namespace FE
{

AnimationModel::AnimationModel(Engine* engine, const std::string& modelName, const std::string& animationName)
    : AnimationModel(engine,
        ModelManager::GetInstance().Get(modelName),
        AnimationManager::GetInstance()->Get(animationName))
{
}

AnimationModel::AnimationModel(Engine* engine, const ModelData* modelData, const Animation* animation)
    : engine_(engine)
{
    assert(engine_ != nullptr);
    assert(modelData != nullptr);
    assert(animation != nullptr);

    // データのセットアップ
    animeModelData_.modelData = modelData;
    animeModelData_.currentAnimation = animation;

    // マテリアル初期化
    materials_.reserve(animeModelData_.modelData->meshes.size());
    for (const auto& mesh : animeModelData_.modelData->meshes)
    {
        MaterialHandle newMaterial = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

        auto& texManager = TextureManager::GetInstance();

        // デフォルト設定
        newMaterial.textureHandle = texManager.Get("white1x1");
        newMaterial.envMapHandle = texManager.Get("skybox"); 
        newMaterial.toonRampHandle = texManager.Get("toonRamp_01");    
        newMaterial.dissolveMapHandle = texManager.Get("white1x1"); 
        newMaterial.normalMapHandle = texManager.Get("white1x1");

        // UV初期化
        newMaterial.uvTransformData.Initialize();
        if (newMaterial.materialData)
        {
            newMaterial.materialData->uvTransform = newMaterial.uvTransformData.matWorld_;
        }

        materials_.push_back(newMaterial);
    }

    // スケルトン・スキンクラスター生成
    skeleton_ = CreateSkeleton(animeModelData_.modelData->rootNode);
    skinCluster_ = CreateSkinCluster(
        engine_->GetGraphicsDevice()->GetDevice(),
        skeleton_,
        *animeModelData_.modelData,
        engine_->GetSRVManager()
    );

    // 初期状態の設定
    animationTime_ = 0.0f;
    isPlaying_ = true; // 生成と同時に再生開始
}

AnimationModel::~AnimationModel()
{
    // SRVの解放
    if (engine_ && engine_->GetSRVManager())
    {
        engine_->GetSRVManager()->FreeSRV(skinCluster_.paletteSrvIndex);
    }
}

void AnimationModel::Update()
{
    // アニメーションが無効、またはデータ不正なら姿勢更新のみして終了
    if (!animeModelData_.currentAnimation || animeModelData_.currentAnimation->duration <= 0.0f)
    {
        UpdateSkeleton(skeleton_);
        UpdateSkinCluster(skinCluster_, skeleton_);
        return;
    }

    // 再生中の場合、時間を進める
    if (isPlaying_ && !isFinished_)
    {
        float duration = animeModelData_.currentAnimation->duration;

        // 経過時間を加算
        animationTime_ += TimeManager::GetInstance()->GetDeltaTime() * speedScale_;

        // 進行度の計算
        float rawT = animationTime_ / duration;

        if (isLoop_)
        {
            // ループ処理: 範囲内に収める
            rawT = std::fmod(rawT, 1.0f);
            if (rawT < 0.0f) rawT += 1.0f; // 逆再生対応

            // 時間変数も範囲内に戻しておく
            animationTime_ = rawT * duration;
        }
        else
        {
            // 非ループ: 終了判定
            if (rawT >= 1.0f)
            {
                rawT = 1.0f;
                animationTime_ = duration;
                isFinished_ = true;
            }
        }

        // イージング適用
        float easedT = Easing::Evaluate(easingType_, rawT);
        float playbackTime = easedT * duration;

        // アニメーションをボーンに適用
        ApplyAnimation(skeleton_, *animeModelData_.currentAnimation, playbackTime);
    }

    // 行列更新
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

void AnimationModel::Draw()
{
    if (!engine_ || !animeModelData_.modelData) return;

    // モデル自体のワールド行列更新
    transform_.UpdateMatrix();

    engine_->GetRendererManager()->SubmitAnimationModel(
        transform_,
        animeModelData_,
        skinCluster_,
        materials_,
        blendMode_,
        renderGroup_,
        baseColor_
    );
}

// ========================================================================
// アニメーション制御
// ========================================================================

void AnimationModel::Play(const std::string& animationName, bool isLoop, float speedScale)
{
    // Managerから検索
    const Animation* anim = AnimationManager::GetInstance()->Get(animationName);

    // 見つかればポインタ版のPlayに投げる
    if (anim)
    {
        Play(anim, isLoop, speedScale);
    }
    else
    {
        // エラーログ
    }
}

void AnimationModel::SetAnimation(const std::string& animationName)
{
    // Managerから検索
    const Animation* anim = AnimationManager::GetInstance()->Get(animationName);

    // 見つかればセット
    if (anim)
    {
        SetAnimation(anim);
    }
}

void AnimationModel::Play(const Animation* animation, bool isLoop, float speedScale)
{
    // ポインタが無効なら無視
    if (!animation) return;

    // すでに同じアニメーションが指定されている場合は、リセットせずに処理を抜ける
    if (animeModelData_.currentAnimation == animation)
    {
        isLoop_ = isLoop;        
        speedScale_ = speedScale; 
        isPlaying_ = true;       
        return;                   
    }

    animeModelData_.currentAnimation = animation;
    isLoop_ = isLoop;
    speedScale_ = speedScale;

    ResetAnimation(); // 新しいアニメーションの時だけリセットがかかる
    isPlaying_ = true;
}

void AnimationModel::SetAnimation(const Animation* animation)
{
    // 違うアニメーションなら切り替え
    if (animeModelData_.currentAnimation != animation)
    {
        animeModelData_.currentAnimation = animation;
        ResetAnimation();
    }
}

void AnimationModel::ResetAnimation()
{
    animationTime_ = 0.0f;
    isFinished_ = false;
}

// ========================================================================
// マテリアル一括設定
// ========================================================================

void AnimationModel::SetUVTransform(const WorldTransform& uvTransform)
{
    for (auto& mat : materials_)
    {
        mat.uvTransformData.translation_ = uvTransform.translation_;
        mat.uvTransformData.rotation_ = uvTransform.rotation_;
        mat.uvTransformData.scale_ = uvTransform.scale_;

        mat.uvTransformData.UpdateMatrix();
        if (mat.materialData)
        {
            mat.materialData->uvTransform = mat.uvTransformData.matWorld_;
        }
    }
}

void AnimationModel::SetTexture(const std::string& textureName)
{
    // 文字列からGPUハンドルを検索して取得
    uint32_t handle = TextureManager::GetInstance().Get(textureName);

    // 全マテリアルに適用
    for (auto& mat : materials_) mat.textureHandle = handle;
}

void AnimationModel::SetEnvironmentMapTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.envMapHandle = handle;
}

void AnimationModel::SetToonRampTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.toonRampHandle = handle;
}

void AnimationModel::SetDissolveTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.dissolveMapHandle = handle;
}

void AnimationModel::SetNormalMapTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.normalMapHandle = handle;
}

void AnimationModel::SetRippleTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.rippleTextureHandle = handle;
}

void AnimationModel::SetPuddleNoiseTexture(const std::string& textureName)
{
    uint32_t handle = TextureManager::GetInstance().Get(textureName);
    for (auto& mat : materials_) mat.puddleNoiseHandle = handle;
}


void AnimationModel::SetColor(const Vector4& color)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->color = color;
    }
}

void AnimationModel::SetColor(uint32_t color)
{
    SetColor(Math::Uint32ToColorVector(color));
}

void AnimationModel::SetBaseColor(uint32_t color) { baseColor_ = Math::Uint32ToColorVector(color); }

void AnimationModel::SetEmissiveIntensity(float intensity)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->emissiveIntensity = intensity;
    }
}

void AnimationModel::SetEnableOutline(bool enable)
{
    int flag = enable ? 1 : 0;
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->enableOutline = flag;
    }
}

void AnimationModel::SetOutlineWidth(float width)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->outlineWidth = width;
    }
}

void AnimationModel::SetOutlineColor(const Vector4& color)
{
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->outlineColor = color;
    }
}

void AnimationModel::SetOutlineColor(uint32_t color)
{
    SetOutlineColor(Math::Uint32ToColorVector(color));
}

void AnimationModel::SetEnableDissolve(bool enable)
{
    int flag = enable ? 1 : 0;
    for (auto& mat : materials_) {
        if (mat.materialData) mat.materialData->enableDissolve = flag;
    }
}

void AnimationModel::UpdateUV()
{
    for (auto& mat : materials_)
    {
        mat.uvTransformData.UpdateMatrix();
        if (mat.materialData)
        {
            mat.materialData->uvTransform = mat.uvTransformData.matWorld_;
        }
    }
}

// ========================================================================
// マテリアル個別設定・ゲッター
// ========================================================================

void AnimationModel::SetMaterialColor(size_t index, const Vector4& color)
{
    if (IsValidMaterialIndex(index) && materials_[index].materialData) {
        materials_[index].materialData->color = color;
    }
}

void AnimationModel::SetMaterialColor(size_t index, uint32_t color)
{
    SetMaterialColor(index, Math::Uint32ToColorVector(color));
}

MaterialData* AnimationModel::GetMaterialData(size_t index)
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return materials_[index].materialData;
}

const MaterialData* AnimationModel::GetMaterialData(size_t index) const
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return materials_[index].materialData;
}

MaterialHandle* AnimationModel::GetMaterialHandle(size_t index)
{
    if (!IsValidMaterialIndex(index)) return nullptr;
    return &materials_[index];
}

Vector4* AnimationModel::GetMaterialColorPtr(size_t index)
{
    if (IsValidMaterialIndex(index) && materials_[index].materialData) {
        return &materials_[index].materialData->color;
    }
    return nullptr;
}

bool AnimationModel::IsValidMaterialIndex(size_t index) const
{
    return index < materials_.size();
}

Vector3 AnimationModel::GetSkinnedVertexPosition(size_t meshIndex, size_t vertexIndex) const
{
    const auto& mesh = animeModelData_.modelData->meshes[meshIndex];

    // オリジナルの頂点座標
    const Vector4& origPos = mesh.vertices[vertexIndex].position;

    // この頂点にかかるウェイト情報
    const auto& influence = skinCluster_.meshInfluences[meshIndex].mappedInfluence[vertexIndex];

    Vector3 skinnedPos = { 0.0f, 0.0f, 0.0f };

    // 影響を受けるボーン（最大4つ）の計算を合成
    for (int i = 0; i < kNumMaxInfluence; ++i)
    {
        float weight = influence.weights[i];
        if (weight <= 0.0f) continue; // ウェイトが0なら計算をスキップ

        int32_t jointIndex = influence.jointIndices[i];

        // パレットから対象ボーンのスキニング用行列を取得
        const Matrix4x4& jointMatrix = skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix;

        // 頂点座標に行列を掛ける
        Vector3 transformedPos;
        transformedPos.x = origPos.x * jointMatrix.m[0][0] + origPos.y * jointMatrix.m[1][0] + origPos.z * jointMatrix.m[2][0] + 1.0f * jointMatrix.m[3][0];
        transformedPos.y = origPos.x * jointMatrix.m[0][1] + origPos.y * jointMatrix.m[1][1] + origPos.z * jointMatrix.m[2][1] + 1.0f * jointMatrix.m[3][1];
        transformedPos.z = origPos.x * jointMatrix.m[0][2] + origPos.y * jointMatrix.m[1][2] + origPos.z * jointMatrix.m[2][2] + 1.0f * jointMatrix.m[3][2];

        // ウェイトを掛けて足し合わせる
        skinnedPos.x += transformedPos.x * weight;
        skinnedPos.y += transformedPos.y * weight;
        skinnedPos.z += transformedPos.z * weight;
    }

    return skinnedPos;
}

}