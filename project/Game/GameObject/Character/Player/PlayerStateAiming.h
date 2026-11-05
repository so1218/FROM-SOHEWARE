#pragma once
#include "Player.h"

class PlayerStateAiming : public State<Player>
{
public:
    void Enter(Player* player) override;
    void Update(Player* player) override;
    void Exit(Player* player) override;

    const char* GetName() const override { return "Aiming"; }
};
