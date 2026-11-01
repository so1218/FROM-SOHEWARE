#pragma once
#include "Collider.h"

class Enemy; 

// 敵の体の大きさと HP。同じ種類の敵で共有する
struct EnemyBodyConfig
{
    int maxHP = 100;
    FS::Vector3 scale = { 1.0f, 1.0f, 1.0f };
    float colliderRadius = 1.0f;
    FS::Vector3 colliderOffset = { 0.0f, 0.0f, 0.0f };
};

// 全ての敵の挙動のベースとなるインターフェース
class IEnemyBehavior
{
public:
    virtual ~IEnemyBehavior() = default;

    virtual void Initialize(Enemy* owner) = 0;
    virtual void Update(Enemy* owner) = 0;
    virtual void DebugDraw(Enemy* owner) = 0;

    // 衝突処理
    virtual void OnCollisionEnter(Enemy* owner, FS::GameObject* hitObject) {}

    // 敵固有のパラメータ・演出
    virtual const EnemyBodyConfig& GetBodyConfig() const = 0;
    virtual std::string GetDamageParticleName() const { return "EnemyDamageParticle"; }

    // 当たった場所に応じてダメージを変える (弱点など)
    virtual int ModifyDamage(Enemy* owner, int damage, const FS::Vector3& hitPoint) { return damage; }

    // 被弾時の固有処理
    virtual void OnTakeDamage(Enemy* owner, int damage, const FS::Vector3& hitPoint, const FS::Vector3& hitNormal) {}

    // 死亡時の固有処理
    virtual void OnDeath(Enemy* owner) {}

    // 倒されずに消えるとき (リセットなど)。追従しているエフェクトや音を片付ける
    virtual void OnDespawn(Enemy* owner) {}
};