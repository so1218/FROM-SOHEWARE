#pragma once
#include "Bullet.h"
#include "Knife.h"
#include "KnifeBullet.h"

enum class BulletType
{
    Knife,
    
};

class BulletFactory 
{
public:
    static std::unique_ptr<Bullet> CreateBullet(BulletType type, const Vector3& pos, const Vector3& dir, int level);
};