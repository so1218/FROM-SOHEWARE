#pragma once
#include "Player.h"

class PlayerStateRoll : public State<Player>
{
public:
    static PlayerStateRoll* GetInstance() 
    {
        static PlayerStateRoll instance;
        return &instance;
    }

    void Enter(Player* p) override;
    void Update(Player* p) override;
    void Exit(Player* p) override;

    std::string GetName() override { return "Roll"; }

private:
    float timer_ = 0.0f;
    const float kRollDuration = 0.4f; // ローリング完了までの時間
    FE::Vector3 rollDirection_;
};