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

    void Enter(Player* p) override;
    void Update(Player* p) override;
    void Exit(Player* p) override {}

    std::string GetName() override { return "Jump"; }
};