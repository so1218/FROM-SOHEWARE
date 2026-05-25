#pragma once
#include "Boss.h"

class BossStateIdle : public State<Boss>
{
public:
    static BossStateIdle* GetInstance()
    {
        static BossStateIdle instance;
        return &instance;
    }

    void Enter(Boss* b) override;
    void Exit(Boss* b) override {}

    void Update(Boss* b) override;

    std::string GetName() override { return "Idle"; }
};