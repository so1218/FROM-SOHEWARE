#pragma once
#include "Weapon.h"

class Knife : public Weapon
{
public:
    Knife();

    void Update(Player* player) override;
    void Shoot(Player* player) override;

    void SetLevel(int level) override;

private:
    float cooldownTimer_ = 0.0f;
    float cooldownMax_ = 1.0f;
};
