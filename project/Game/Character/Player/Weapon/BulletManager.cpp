#include "BulletManager.h"

void BulletManager::SpawnBullet(BulletType type, const Vector3& pos, const Vector3& dir, int level)
{
    auto bullet = BulletFactory::CreateBullet(type, pos, dir, level);
    if (bullet) 
    {
        bullets_.push_back(std::move(bullet));
    }
}

void BulletManager::UpdateAll()
{
    for (auto& b : bullets_) {
        b->Update();
    }

    // 死亡した弾を削除
    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(),
            [](const std::unique_ptr<Bullet>& b) {
                return b->IsDead();
            }),
        bullets_.end()
    );
}

void BulletManager::DrawAll()
{
    for (auto& b : bullets_)
    {
        b->Draw();
    }
}