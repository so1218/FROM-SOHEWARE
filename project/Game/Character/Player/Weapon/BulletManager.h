#pragma once
#include "BulletFactory.h"
#include "Vector3.h"

class BulletManager 
{
public:
    static BulletManager* GetInstance()
    {
        static BulletManager instance;
        return &instance;
    }

    void SpawnBullet(BulletType type, const Vector3& pos, const Vector3& dir, int level);

    void UpdateAll();

    void DrawAll();

private:
    std::vector<std::unique_ptr<Bullet>> bullets_;
};