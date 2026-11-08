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
    // Zombie,       
};

class Enemy : public FS::GameObject
{
public:
    Enemy(FS::Engine* engine, int id, EnemyType type, std::unique_ptr<IEnemyBehavior> behavior, const std::string& parentGroupName);
    ~Enemy() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

    // 被弾処理。hitCollider は弾が当たったコライダー (弱点の判定に使う)
    void TakeDamage(int damage, const FS::Vector3& hitPoint, const FS::Vector3& hitNormal, const FS::Collider* hitCollider = nullptr);
    void Die();
    void Despawn();
    // 倒されて消えたか。Despawn で消えたときは false
    bool IsDefeated() const { return isDefeated_; }

    // アクセサ
    FS::Engine* GetEngine() const { return engine_; }
    FS::PropertyBinder* GetBinder() const { return binder_.get(); }
    FS::WorldTransform& GetTransform() { return model_->GetTransform(); }
    FS::Model* GetModel() const { return model_.get(); }
    EnemyType GetType() const { return type_; }

    // Transform更新を本体に反映させるヘルパー
    void SyncTransform() { SetTransform(model_->GetTransform()); }

private:
    FS::Engine* engine_;
    int id_;
    EnemyType type_;

    std::unique_ptr<FS::Model> model_;
    FS::Collider* collider_ = nullptr; // 所有は GameObject (AddCollider で作成)
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::ParticleEmitter> damageParticle_ = nullptr;
    FS::ParticleEmitter* damageParticlePtr_ = nullptr;

    // 振る舞いを保持するポインタ
    std::unique_ptr<IEnemyBehavior> behavior_;

    int hp_ = 100;
    bool isDefeated_ = false;

};
