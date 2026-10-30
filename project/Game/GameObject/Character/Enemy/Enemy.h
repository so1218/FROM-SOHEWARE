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
    // Fireball,   
    // Pop         
};

class Enemy : public FS::GameObject
{
public:
    Enemy(FS::Engine* engine, int id, EnemyType type, const std::string& parentGroupName);
    ~Enemy() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

    // 被弾処理
    void TakeDamage(int damage, const FS::Vector3& hitPoint, const FS::Vector3& hitNormal);

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

    // 共通パラメータ
    int hp_ = 100;
    FS::Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
    float colliderRadius_ = 1.0f;
    FS::Vector3 colliderOffset_ = { 0.0f, 0.0f, 0.0f };

};
