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

enum class PropColliderType
{
    Sphere = 0,
    Box = 1,
};

// ステージに置く小物 (プロップ) 1 つ分
class EnvironmentProp : public FS::GameObject
{
public:
    EnvironmentProp(FS::Engine* engine, int id, const std::string& parentGroupName);
    ~EnvironmentProp() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;
    void OnCollisionStay(FS::Collider* mine, FS::Collider* other) override;

    // 設定に合わせて、当たり判定とパーティクルを作ったり消したりする
    void ApplySettings();

    // 拾って消えていたら、もう一度出してパーティクルも付け直す
    void ResetState();

    // シーンの途中でこのプロップを消す前に呼ぶ
    void ReleaseEmitters();

    // 今のマテリアルの値を保存データに書く
    void SyncMaterialsToJSON();

    // 形を借りる元のモデルを差し替える
    void ChangeMasterModel(FS::Model* newMaster);

    // 番号を変え、保存先もその番号に移す
    void ReassignID(int newID);

    int GetID() const { return id_; }
    FS::Model* GetModel() const { return model_.get(); }
    FS::WorldTransform& GetTransformRef() { return model_->GetTransform(); }

    std::string GetDisplayName() const {
        return propCustomName_.empty() ? prefabName_ : propCustomName_;
    }

    void SetMasterModel(FS::Model* master) { masterModel_ = master; }
    void SetPrefabName(const std::string& name) { prefabName_ = name; }

private:
    PropBehavior GetBehavior() const { return static_cast<PropBehavior>(propBehavior_); }
    PropColliderType GetColliderType() const { return static_cast<PropColliderType>(colliderType_); }

    // 保存する値を PropertyBinder に登録する
    void SetupProperties();

    // パーティクルを使う設定なら作り、使わない設定なら消す
    void UpdateParticleEmitter(
        bool hasParticle,
        const std::string& particleName,
        bool isFollowing,
        FS::ParticleEmitter*& outEmitter
    );

private:
    FS::Engine* engine_ = nullptr;
    FS::Model* masterModel_ = nullptr;
    int id_ = -1;
    std::string parentGroupName_;
    std::string prefabName_;
    std::string modelName_ = "cube";
    std::string propCustomName_;

    std::unique_ptr<FS::Model> model_;
    FS::Collider* collider_ = nullptr; 
    std::unique_ptr<FS::PropertyBinder> binder_;

    FS::Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 振る舞いと当たり判定。ImGui の選択と保存は int で行うので int で持ち、使うときは GetBehavior / GetColliderType で読む
    int propBehavior_ = static_cast<int>(PropBehavior::None);
    bool hasCollider_ = false;
    int colliderType_ = static_cast<int>(PropColliderType::Sphere);
    float colliderRadius_ = 1.0f;
    FS::Vector3 colliderSize_ = { 1.0f, 1.0f, 1.0f };
    FS::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

    // 点光源
    bool hasLight_ = false;
    FS::Vector4 lightColor_ = { 1.0f, 0.5f, 0.0f, 1.0f };
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;

    // パーティクル 1 つ目
    bool hasParticle_ = false;
    std::string particleName_ = "enemyAura";
    FS::ParticleEmitter* activeEmitter_ = nullptr;
    bool isParticleFollowing_ = true;

    // パーティクル 2 つ目
    bool hasParticle2_ = false;
    std::string particleName2_ = "enemyAura";
    FS::ParticleEmitter* activeEmitter2_ = nullptr;
    bool isParticleFollowing2_ = true;
};