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

    void Enter(Player* p) override {}
    void Exit(Player* p) override {}

    void Update(Player* p) override;

    std::string GetName() override { return "Normal"; }
};