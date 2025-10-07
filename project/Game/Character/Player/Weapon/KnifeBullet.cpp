#include "KnifeBullet.h"
#include "ModelHandle.h"

KnifeBullet::KnifeBullet(Engine* engine, Camera* camera, const Vector3& pos, const Vector3& dir, int level)
    : Bullet(pos, dir, level)
{
    engine_ = engine;
    camera_ = camera;

    speed_ = 10.0f + level * 2.0f;
    lifeTime_ = 5.0f;
    isDead_ = false;
    knifeModel_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::sphere)));
    // 追加パラメータ設定
}

void KnifeBullet::Update()
{
    // 独自挙動あればここで実装
    Bullet::Update();
}

void KnifeBullet::Draw()
{
    knifeModel_->Draw();
}