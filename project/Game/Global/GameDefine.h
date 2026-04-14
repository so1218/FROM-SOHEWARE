#pragma once
#include <cstdint>

namespace ObjectUpdateOrder
{
    enum Priority : int
    {
        Field = 10,
        Player = 30,
        Default = 30,
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