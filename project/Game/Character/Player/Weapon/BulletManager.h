#pragma once
#include "BulletFactory.h"
#include "GameObjectManager.h"
#include "Vector3.h"

class BulletManager 
{
public:
    static BulletManager* GetInstance()
    {
        static BulletManager instance;
        return &instance;
    }

    void Initialize(GameObjectManager* objectManager);

    void SpawnBullet(Engine* engine, Camera* camera, BulletType type, const Vector3& pos, const Vector3& dir, int level);

private:
    std::vector<std::unique_ptr<Bullet>> bullets_;
    GameObjectManager* objectManager_;
};