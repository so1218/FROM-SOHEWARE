#pragma once
#include "Player.h"

class PlayerStateCrouch : public State<Player>
{
public:
    void Update(Player* player) override;

    const char* GetName() const override { return "Crouch"; }
};
