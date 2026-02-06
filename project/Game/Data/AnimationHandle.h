#pragma once

#include "Engine.h"

// アニメーションID
enum class AnimationID
{
    // プレイヤー
    playerIdle,
    playerWalk,

    count
};

class AnimationHandle
{
public:

    static void Initialize();
    static const Animation* Get(AnimationID id);

private:
    static std::array<Animation, static_cast<size_t>(AnimationID::count)> animations_;
    static bool initialized_;

    static constexpr std::array<std::pair<AnimationID, const char*>, static_cast<size_t>(AnimationID::count)> animationDefinitions_ =
    {
        {
            // プレイヤー
            { AnimationID::playerIdle,     "Assets/Models/Characters/Player/playerIdle.gltf" },
            { AnimationID::playerWalk,     "Assets/Models/Characters/Player/playerWalk.gltf" },
        }
    };
};
