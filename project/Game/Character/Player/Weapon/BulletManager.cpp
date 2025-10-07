#include "BulletManager.h"

void BulletManager::Initialize(GameObjectManager* objectManager)
{
    objectManager_ = objectManager;
}

void BulletManager::SpawnBullet(Engine* engine, Camera* camera, BulletType type, const Vector3& pos, const Vector3& dir, int level)
{
    auto bullet = BulletFactory::CreateBullet(engine, camera, type, pos, dir, level);
    if (bullet) 
    {
        objectManager_->AddObject(std::move(bullet));
    }
}
