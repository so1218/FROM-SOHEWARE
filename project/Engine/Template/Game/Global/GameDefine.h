#pragma once
#include <cstdint>
#include <string>

// 更新の順番 (小さいほど先)
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

// オブジェクトの種類
namespace ObjectTag
{
    enum ID : uint32_t
    {
        None = 0,
        Player,
        Enemy,
    };
}

// シーンの名前。保存したパラメータのファイル名にも使われる
namespace SceneName
{
    inline const std::string Main = "MainScene";
}
