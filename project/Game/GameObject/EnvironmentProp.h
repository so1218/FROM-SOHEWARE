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
};

class EnvironmentProp : public FE::GameObject
{
public:
    EnvironmentProp(FE::Engine* engine, int id, const std::string& parentGroupName);
    ~EnvironmentProp() override; // シーン切り替え時のライト自動返却を保証

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;
    void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;
    void OnCollisionStay(FE::Collider* mine, FE::Collider* other) override;
    void OnCollisionExit(FE::Collider* mine, FE::Collider* other) override;

    // 設定が変更された際に、コライダーやライトの有効/無効を再構築
    void ApplySettings();

    FE::Model* GetModel() { return model_.get(); };

    // IDを振り直し、JSONの保存先を更新する関数
    void ReassignID(int newID);

    // 自分のIDを返す関数
    int GetID() const { return id_; }

    // カスタム名が空なら Prefab 名を返し、設定されていればカスタム名を返す場合
    std::string GetDisplayName() const {
        return propCustomName_.empty() ? prefabName_ : propCustomName_;
    }

    // ギズモ操作のために、モデルのTransformの参照を返すゲッター
    FE::WorldTransform& GetTransformRef() {
        return model_->GetTransform();
    }

    void SetMasterModel(FE::Model* master) { masterModel_ = master; }

    void SyncMaterialsToJSON();

    // 自分が属するプレハブ名を保持
    void SetPrefabName(const std::string& name) { prefabName_ = name; }
    const std::string& GetPrefabName() const { return prefabName_; }

    void ChangeMasterModel(FE::Model* newMaster);

private:
    FE::Engine* engine_;
    FE::Model* masterModel_ = nullptr;
    int id_;
    std::string parentGroupName_;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    // PropertyBinderで保存・変更するパラメータ
    int propBehavior_ = 0;
    bool hasCollider_ = false;

    int colliderType_ = 0; 
    float colliderRadius_ = 1.0f;
    FE::Vector3 colliderSize_ = { 0.5f, 0.5f, 0.5f };
    FE::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

    // ライト設定関連
    bool hasLight_ = false;
    int pointLightIndex_ = -1;
    FE::Vector4 lightColor_ = { 1.0f, 0.5f, 0.0f, 1.0f }; 
    float lightIntensity_ = 5.0f;
    float lightRadius_ = 10.0f;
    float lightVolumetricScatteringIntensity_ = 1.0f;

    std::string modelName_ = "cube";
    std::string propCustomName_ = ""; // ユーザーが自由に付けられる名前

    void SetupProperties(); // プロパティの登録をひとまとめにする関数

    bool hasParticle_ = false;
    std::string particleName_ = "enemyAura";
    FE::ParticleEmitter* activeEmitter_ = nullptr;
    bool isParticleFollowing_ = true; // 追従するかどうかのフラグ

    bool hasParticle2_ = false;
    std::string particleName2_ = "enemyAura";
    FE::ParticleEmitter* activeEmitter2_ = nullptr;
    bool isParticleFollowing2_ = true;

    std::string prefabName_;
    FE::Vector4 baseColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};