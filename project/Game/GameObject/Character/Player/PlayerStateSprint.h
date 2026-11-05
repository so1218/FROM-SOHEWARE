#pragma once
#include "Player.h"

// ダッシュ
class PlayerStateSprint : public State<Player>
{
public:
    void Enter(Player* player) override;
    void Update(Player* player) override;

    const char* GetName() const override { return "Sprint"; }
};
