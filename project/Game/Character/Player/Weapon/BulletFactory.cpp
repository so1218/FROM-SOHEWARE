#include "BulletFactory.h"

std::unique_ptr<Bullet> BulletFactory::CreateBullet(BulletType type, const Vector3& pos, const Vector3& dir, int level) 
{
    switch (type)
    {
    case BulletType::Knife:
        return std::make_unique<KnifeBullet>(pos, dir, level);
        // 他の弾もここに追加可能
    default:
        return nullptr;
    }
}