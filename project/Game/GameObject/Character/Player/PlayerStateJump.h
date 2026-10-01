#pragma once
#include "Player.h"

class PlayerStateJump : public State<Player>
{
public:
    static PlayerStateJump* GetInstance()
    {
        static PlayerStateJump instance;
        return &instance;
    }

    void Enter(Player* player) override;
    void Update(Player* player) override;
    void Exit(Player* player) override {}

    std::string GetName() override { return "Jump"; }
};