#pragma once
#include "Player.h"

// 回避。入力した方向 (入力がなければ向いている方向) へ素早く移動する
class PlayerStateDodge : public State<Player>
{
public:
    static PlayerStateDodge* GetInstance()
    {
        static PlayerStateDodge instance;
        return &instance;
    }

    void Enter(Player* player) override;
    void Update(Player* player) override;
    void Exit(Player* player) override {}

    std::string GetName() override { return "Dodge"; }
};
