#pragma once
#include "Player.h"

// 回避
class PlayerStateDodge : public State<Player>
{
public:
    void Enter(Player* player) override;
    void Update(Player* player) override;

    const char* GetName() const override { return "Dodge"; }

private:
    FS::Vector3 direction_ = { 0.0f, 0.0f, 1.0f };
    float elapsedTime_ = 0.0f;
};
