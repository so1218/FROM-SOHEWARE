#pragma once
#include "Player.h"

class PlayerStateNormal : public State<Player>
{
public:
    static PlayerStateNormal* GetInstance()
    {
        static PlayerStateNormal instance;
        return &instance;
    }

    void Enter(Player* player) override {}
    void Exit(Player* player) override {}
    void Update(Player* player) override;

    std::string GetName() override { return "Normal"; }
};