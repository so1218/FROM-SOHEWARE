#pragma once
#include "Boss.h"

class BossStateJumpAttack : public State<Boss>
{
public:
    static BossStateJumpAttack* GetInstance()
    {
        static BossStateJumpAttack instance;
        return &instance;
    }

    void Enter(Boss* b) override;
    void Exit(Boss* b) override {}

    void Update(Boss* b) override;

    std::string GetName() override { return "JumpAttack"; }
};