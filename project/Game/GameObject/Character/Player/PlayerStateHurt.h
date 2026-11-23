#pragma once
#include "Player.h"

class PlayerStateHurt : public State<Player>
{
public:
    explicit PlayerStateHurt(const FS::Vector3& knockbackDirection);

    void Enter(Player* player) override;
    void Update(Player* player) override;

    const char* GetName() const override { return "Hurt"; }

private:
    FS::Vector3 direction_;
    float elapsedTime_ = 0.0f;
};
