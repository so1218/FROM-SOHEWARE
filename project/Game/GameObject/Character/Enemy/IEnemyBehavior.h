#pragma once
#include "Collider.h"

class Enemy; 

// 全ての敵の挙動のベースとなるインターフェース
class IEnemyBehavior
{
public:
    virtual ~IEnemyBehavior() = default;

    virtual void Initialize(Enemy* owner) = 0;
    virtual void Update(Enemy* owner) = 0;
    virtual void DebugDraw(Enemy* owner) = 0;

    // 衝突処理
    virtual void OnCollisionEnter(Enemy* owner, FE::GameObject* hitObject) {}

    // 敵固有のパラメータ・演出
    virtual int GetInitialHP() const { return 100; }
    virtual std::string GetDamageParticleName() const { return "EnemyDamageParticle"; }

    // 被弾時の固有処理
    virtual void OnTakeDamage(Enemy* owner, int damage, const FE::Vector3& hitPoint, const FE::Vector3& hitNormal) {}

    // 死亡時の固有処理
    virtual void OnDeath(Enemy* owner) {}
};