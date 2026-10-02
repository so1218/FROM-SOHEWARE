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
        newMaterial.envMapHandle = texManager.Get("pureSky"); 
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
        engine_->GetSRVManager()->FreeSRV(skinCluster_.prevPaletteSrvIndex);
    }
}

void AnimationModel::Update()
{
    if (!animeModelData_.currentAnimation || animeModelData_.currentAnimation->duration <= 0.0f)
    {
        UpdateSkeleton(skeleton_);
        UpdateSkinCluster(skinCluster_, skeleton_);
        return;
    }

    if (isPlaying_ && !isFinished_)
    {
        // 実際の時間と、アニメーション用の時間を分ける
        float realDeltaTime = TimeManager::GetInstance()->GetDeltaTime();
        float animDeltaTime = realDeltaTime * speedScale_;

        // 新しいアニメーションの時間を進める
        float duration = animeModelData_.currentAnimation->duration;
        animationTime_ += animDeltaTime; 

        float rawT = animationTime_ / duration;
        if (isLoop_)
        {
            rawT = std::fmod(rawT, 1.0f);
            if (rawT < 0.0f) rawT += 1.0f;
            animationTime_ = rawT * duration;
        }
        else if (rawT >= 1.0f)
        {
            rawT = 1.0f;
            animationTime_ = duration;
            isFinished_ = true;
        }

        float easedT = Easing::Evaluate(easingType_, rawT);
        float playbackTime = easedT * duration;

        // ブレンド中の処理
        if (isBlending_ && prevAnimation_)
        {
            // ブレンドタイマーに realDeltaTime を足す
            blendTimer_ += realDeltaTime;
            float blendFactor = blendTimer_ / blendDuration_;

            // 旧アニメーションの時間更新
            prevAnimationTime_ += animDeltaTime;
            if (prevAnimation_->duration > 0.0f)
            {
                prevAnimationTime_ = std::fmod(prevAnimationTime_, prevAnimation_->duration);
            }

            if (blendFactor >= 1.0f)
            {
                // ブレンド完了
                isBlending_ = false;
                prevAnimation_ = nullptr;
            }
            else
            {
                // ブレンド中
                ApplyBlendAnimation(
                    skeleton_,
                    *prevAnimation_, prevAnimationTime_,
                    *animeModelData_.currentAnimation, playbackTime,
                    blendFactor
                );
            }
        }

        // 通常時
        if (!isBlending_)
        {
            ApplyAnimation(skeleton_, *animeModelData_.currentAnimation, playbackTime);
        }
    }

    // 行列更新とスキニング更新はそのまま実行
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

void AnimationModel::Play(const std::string& animationName, bool isLoop, float speedScale, float blendTime)
{
    const Animation* anim = AnimationManager::GetInstance()->Get(animationName);
    if (anim)
    {
        Play(anim, isLoop, speedScale, blendTime);
    }
}

void AnimationModel::Play(const Animation* animation, bool isLoop, float speedScale, float blendTime)
{
    if (!animation) return;

    // すでに同じアニメーションが再生中の場合はパラメータのみ更新
    if (animeModelData_.currentAnimation == animation)
    {
        isLoop_ = isLoop;
        speedScale_ = speedScale;
        isPlaying_ = true;
        return;
    }

    // ブレンド時間が指定されており、現在アニメーションが再生されている場合はブレンドを開始
    if (blendTime > 0.0f && animeModelData_.currentAnimation != nullptr)
    {
        prevAnimation_ = animeModelData_.currentAnimation;
        prevAnimationTime_ = animationTime_; // 遷移開始時点の時間を保持
        blendDuration_ = blendTime;
        blendTimer_ = 0.0f;
        isBlending_ = true;
    }
    else
    {
        isBlending_ = false;
    }

    animeModelData_.currentAnimation = animation;
    isLoop_ = isLoop;
    speedScale_ = speedScale;

    ResetAnimation(); // 新しいアニメーションの時間を0にリセット
    isPlaying_ = true;
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

void AnimationModel::AddJointRotationOffset(const std::string& jointName, const Quaternion& offsetRotation)
{
    auto it = skeleton_.jointMap.find(jointName);
    if (it != skeleton_.jointMap.end())
    {
        size_t jointIndex = it->second;
        // アニメーションによる元の回転に、オフセット回転を合成 (Quaternion乗算)
        skeleton_.joints[jointIndex].transform.rotationQuaternion_ =
            skeleton_.joints[jointIndex].transform.rotationQuaternion_ * offsetRotation;
    }
}

void AnimationModel::PostUpdateSkeleton()
{
    // 追加したボーン回転を子ボーン（腕や手首）へ伝搬計算
    UpdateSkeleton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
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

float AnimationModel::GetNormalizedTime() const
{
    if (!animeModelData_.currentAnimation || animeModelData_.currentAnimation->duration <= 0.0f)
    {
        return 0.0f;
    }
    return animationTime_ / animeModelData_.currentAnimation->duration;
}

Matrix4x4 AnimationModel::GetJointWorldMatrix(const std::string& jointName) const
{
    // 取得される直前の最新 transform_ で行列を自動更新
    const_cast<WorldTransform&>(transform_).UpdateMatrix();

    // ボーンの名前でマップを検索
    auto it = skeleton_.jointMap.find(jointName);

    // 見つかった場合
    if (it != skeleton_.jointMap.end())
    {
        size_t jointIndex = (*it).second;
        Matrix4x4 boneSkeletonSpaceMatrix = skeleton_.joints[jointIndex].skeletonSpaceMatrix;

        return boneSkeletonSpaceMatrix * transform_.matWorld_;
    }

    return transform_.matWorld_;
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