#pragma once
#include "Player.h"

class PlayerStateNormal : public State<Player>
{
public:
    void Update(Player* player) override;

    const char* GetName() const override { return "Normal"; }
};
