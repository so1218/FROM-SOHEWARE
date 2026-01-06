#pragma once

#include "CollisionManager.h"

class Engine;
class Player;

enum class WeaponType
{
    Knife,
    Axe,
};

class Weapon
{
protected:
    Engine* engine_; 
    Player* player_;  

    int level_ = 1;
    int maxLevel_ = 6;
    float damage_ = 10.0f;
    float cooldown_ = 2.0f;       // 攻撃のクールダウン時間
    float cooldownTimer_ = 0.0f;  // 現在のクールダウン残り時間
    int projectileCount_ = 1;
    float areaSize_ = 1.0f;

    // ベースパラメータ (レベル1の値)
    float damageBase_ = 20.0f;
    float cooldownBase_ = 1.5f;
    int projectileCountBase_ = 1;

    // 当たり判定サイズ
    Vector3 collisionSize_ = { 0.2f, 0.2f, 0.2f };

public:
    Weapon(Engine* engine, Player* owner);
    virtual ~Weapon() {} 

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
    virtual void DebugDraw() = 0;
    virtual void LevelUp() = 0;
    virtual void ApplyLevelEffects() = 0;
    virtual void AddCollidersToManager(CollisionManager* manager) = 0;
    // 武器の種類を取得する関数
    virtual WeaponType GetType() const = 0;
    // レベルが最大かどうか
    virtual bool IsMaxLevel() const { return level_ >= maxLevel_; }

    void SetLevel(int level) { level_ = level; }
};