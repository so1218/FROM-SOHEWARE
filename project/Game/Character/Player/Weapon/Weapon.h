#pragma once
#include "Player.h"

class Weapon
{
public:
    virtual ~Weapon() = default;
    virtual void Update(Player* player) = 0;
    virtual void Shoot(Player* player) = 0;

	virtual void SetLevel(int level) = 0;

protected:
    int level_ = 1;
};