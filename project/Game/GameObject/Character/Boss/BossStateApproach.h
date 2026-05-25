#pragma once
#include "Boss.h"

class BossStateApproach : public State<Boss>
{
public:
    static BossStateApproach* GetInstance()
    {
        static BossStateApproach instance;
        return &instance;
    }

    void Enter(Boss* b) override {}
    void Exit(Boss* b) override {}

    void Update(Boss* b) override;

    std::string GetName() override { return "Approach"; }
};