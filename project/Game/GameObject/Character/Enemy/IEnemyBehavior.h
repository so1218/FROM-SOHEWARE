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
};