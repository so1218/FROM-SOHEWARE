#pragma once
#include "Player.h"

class PlayerStateJump : public State<Player>
{
public:
    void Enter(Player* player) override;
    void Update(Player* player) override;

    const char* GetName() const override { return "Jump"; }
};
