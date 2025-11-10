#pragma once

#include "CollisionManager.h"

class Engine;
class Player;

enum class WeaponType
{
    Knife,
    Garlic,
    Axe,
    Bible,
    FireWand,
    MagicMissile
};

class Weapon
{
protected:
    Engine* engine_; 
    Player* player_;  

    int level_ = 1;
    float damage_ = 10.0f;
    float cooldown_ = 2.0f;       // 攻撃のクールダウン時間
    float cooldownTimer_ = 0.0f;  // 現在のクールダウン残り時間
    int projectileCount_ = 1;
    float areaSize_ = 1.0f;

public:
    Weapon(Engine* engine, Player* owner);
    virtual ~Weapon() {} 

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
    virtual void DebugDraw() = 0;
    virtual void LevelUp() = 0;
    virtual void AddCollidersToManager(CollisionManager* manager) = 0;

    void SetLevel(int level) { level_ = level; }
};