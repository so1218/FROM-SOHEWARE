#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"
#include "ParticleEmitter.h"
#include "IEnemyBehavior.h"

enum class EnemyType
{
    Floating,
    // Zombie,     // 将来追加
    // Fireball,   // 将来追加
    // Pop         // 将来追加
};

class Enemy : public FE::GameObject
{
public:
    Enemy(FE::Engine* engine, int id, EnemyType type, const std::string& parentGroupName);
    ~Enemy() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

    // 被弾処理
    void TakeDamage(int damage, const FE::Vector3& hitPoint, const FE::Vector3& hitNormal);

    // アクセサ
    FE::Engine* GetEngine() const { return engine_; }
    FE::PropertyBinder* GetBinder() const { return binder_.get(); }
    FE::WorldTransform& GetTransform() { return model_->GetTransform(); }
    FE::Model* GetModel() const { return model_.get(); }
    EnemyType GetType() const { return type_; }

    // Transform更新を本体に反映させるヘルパー
    void SyncTransform() { SetTransform(model_->GetTransform()); }

private:
    FE::Engine* engine_;
    int id_;
    EnemyType type_;

    std::unique_ptr<FE::Model> model_;
    std::unique_ptr<FE::Collider> collider_;
    std::unique_ptr<FE::PropertyBinder> binder_;
    std::unique_ptr<FE::ParticleEmitter> auraEmitter_ = nullptr;
    std::unique_ptr<FE::ParticleEmitter> damageParticle_ = nullptr;
    FE::ParticleEmitter* damageParticlePtr_ = nullptr;

    // 振る舞いを保持するポインタ
    std::unique_ptr<IEnemyBehavior> behavior_;

    // 共通パラメータ
    int hp_ = 100;
    FE::Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
    float colliderRadius_ = 1.0f;
    FE::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

};
