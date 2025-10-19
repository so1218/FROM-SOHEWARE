#pragma once
#include "GameObject.h"
#include "TimeManager.h"

class Bullet : public GameObject 
{
public:
    Bullet(const Vector3& pos, const Vector3& dir, int level)
        : position_(pos), direction_(dir), level_(level)
    {
        lifeTime_ = 5.0f;
        isDead_ = false;
    }

    virtual void Update() override 
    {
        position_ += direction_ * speed_ * TimeManager::GetInstance()->GetDeltaTime();

        // 寿命減少
        lifeTime_ -= TimeManager::GetInstance()->GetDeltaTime();
        if (lifeTime_ <= 0.0f) 
        {
            isDead_ = true;
        }
    }

    virtual void Draw() override
    {
        // 描画処理
    }

    virtual GameObjectType GetType() const override { return GameObjectType::Bullet; }

    std::vector<std::string> GetGlobalVariableGroupName() const { return { "Bullet" }; }

    bool IsDead() const { return isDead_; }

protected:
    Vector3 position_;
    Vector3 direction_;
    float speed_ = 5.0f;
    int level_;

    float lifeTime_ = 0.0f;  // 弾の寿命（秒）
    bool isDead_ = false;    // 削除対象かどうか

};