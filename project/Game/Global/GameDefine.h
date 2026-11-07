#pragma once
#include <cstdint>

namespace ObjectUpdateOrder
{
    enum Priority
    {
        Field = 10,
        Player = 30,
        Default = 50,
        Effect = 60,
        UI = 90,
    };
}

namespace ObjectTag
{
    enum ID : uint32_t
    {
        None = 0,
        Player,
        Enemy,
        PlayerAttack,
        EnemyAttack, 
        Item,
    };
}

namespace SceneName
{
    inline const std::string Play = "PlayScene";
    inline const std::string Test = "TestScene";
}