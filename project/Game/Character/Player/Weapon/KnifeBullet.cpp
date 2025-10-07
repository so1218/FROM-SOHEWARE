#include "KnifeBullet.h"

KnifeBullet::KnifeBullet(const Vector3& pos, const Vector3& dir, int level)
    : Bullet(pos, dir, level)
{
    speed_ = 10.0f + level * 2.0f;
    // 追加パラメータ設定
}

void KnifeBullet::Update()
{
    // 独自挙動あればここで実装
    Bullet::Update();
}

void KnifeBullet::Draw()
{
    // 独自描画
    Bullet::Draw();
}