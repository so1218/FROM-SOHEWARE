#pragma once

// ゲーム全体で使う乱数
namespace GameRandom
{

inline std::mt19937& GetEngine()
{
    static std::mt19937 engine{ std::random_device{}() };
    return engine;
}

// min 以上 max 以下の値
inline float Range(float min, float max)
{
    if (min > max)
    {
        std::swap(min, max);
    }
    return std::uniform_real_distribution<float>(min, max)(GetEngine());
}

}
