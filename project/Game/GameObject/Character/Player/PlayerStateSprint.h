#pragma once
#include "Player.h"

// ダッシュ。スタミナを使い続け、息切れすると通常に戻る
class PlayerStateSprint : public State<Player>
{
public:
    static PlayerStateSprint* GetInstance()
    {
        static PlayerStateSprint instance;
        return &instance;
    }

    void Enter(Player* player) override;
    void Update(Player* player) override;
    void Exit(Player* player) override {}

    std::string GetName() override { return "Sprint"; }
};
