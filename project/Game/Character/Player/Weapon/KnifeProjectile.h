#pragma once
#include "Model.h" 
#include "Engine.h"
#include "Camera.h"  
#include "Vector3.h" 

class KnifeProjectile
{
public:
    KnifeProjectile(Engine* engine, Camera* camera, const Vector3& startPos, const Vector3& direction);
    ~KnifeProjectile();

    void Update(float deltaTime);
    void Draw();
    bool IsDead() const { return lifetime_ <= 0.0f; }

    // 多分コライダー設定なども必要になるはず
    // void OnCollision(); 

    void SetSpeed(float speed) { speed_ = speed; }
    void SetLifetime(float lifetime) { lifetime_ = lifetime; }

private:
    std::unique_ptr<Model> model_;
    Vector3 direction_ = { 0.0f, 0.0f, 1.0f };
    float speed_ = 1.0f;     // ナイフの飛翔速度
    float lifetime_ = 3.0f;  // ナイフが消えるまでの時間（秒）
};