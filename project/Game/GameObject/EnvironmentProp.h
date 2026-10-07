#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Collider.h"
#include "Model.h"
#include "ParticleEmitter.h"

enum class PropBehavior 
{
    None = 0,         // 何もしない
    Disappear = 1,    // 拾って消える
    PushBack = 2,     // 押し戻し
};

/// ステージ上の配置物（プロップ）を管理するクラス
class EnvironmentProp : public FE::GameObject
{
public:
    EnvironmentProp(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~EnvironmentProp() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;
    void OnCollisionStay(FE::Collider* mine, FE::Collider* other) override;
    void OnCollisionExit(FE::Collider* mine, FE::Collider* other) override;

    // 各種コンポーネントの生成・破棄状態を適用
    void ApplySettings();

    // JSONへマテリアルパラメータを同期
    void SyncMaterialsToJSON();

    // 参照元モデルの変更とモデルデータの再割り当て
    void ChangeMasterModel(FE::Model* newMaster);

    // ID更新およびプロパティ保存先パスの再設定
    void ReassignID(int newID);

    // ゲッター / セッター
    int GetID() const { return id_; }
    FE::Model* GetModel() const { return model_.get(); }
    FE::WorldTransform& GetTransformRef() { return model_->GetTransform(); }

    std::string GetDisplayName() const {
        return propCustomName_.empty() ? prefabName_ : propCustomName_;
    }

    void SetMasterModel(FE::Model* master) { masterModel_ = master; }
    void SetPrefabName(const std::string& name) { prefabName_ = name; }
    const std::string& GetPrefabName() const { return prefabName_; }

private:
    // プロパティ自動バインドの初期化
    void SetupProperties();

    // パーティクルエミッターの生成・破棄制御
    void UpdateParticleEmitter(
        bool hasParticle,
        const std::string& particleName,
        bool isFollowing,
        FE::ParticleEmitter*& outEmitter
    );

private:
    FE::Engine* engine_ = nullptr;
    FE::Model* masterModel_ = nullptr;
    int id_ = -1;
    std::string parentGroupName_;
    std::string prefabName_;
    std::string modelName_ = "cube";
    std::string propCustomName_;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    FE::Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Behavior & Collision
    int propBehavior_ = 0;
    bool hasCollider_ = false;
    int colliderType_ = 0;
    float colliderRadius_ = 1.0f;
    FE::Vector3 colliderSize_ = { 0.5f, 0.5f, 0.5f };
    FE::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

    // Light Properties
    bool hasLight_ = false;
    FE::Vector4 lightColor_ = { 1.0f, 0.5f, 0.0f, 1.0f };
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;

    // Particle Slot 1
    bool hasParticle_ = false;
    std::string particleName_ = "enemyAura";
    FE::ParticleEmitter* activeEmitter_ = nullptr;
    bool isParticleFollowing_ = true;

    // Particle Slot 2
    bool hasParticle2_ = false;
    std::string particleName2_ = "enemyAura";
    FE::ParticleEmitter* activeEmitter2_ = nullptr;
    bool isParticleFollowing2_ = true;
};