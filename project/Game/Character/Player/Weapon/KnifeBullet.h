#pragma once
#include "Bullet.h"

class KnifeBullet : public Bullet 
{
public:
    KnifeBullet(const Vector3& pos, const Vector3& dir, int level);

    void Update() override;

    void Draw() override;

    const char* GetGlobalVariableGroupName() const override { return "KnifeBullet"; }
};