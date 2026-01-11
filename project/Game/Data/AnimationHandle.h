#pragma once

#include "Engine.h"

// アニメーションID
enum class AnimationID
{
    // 基本的なアニメーション
    player,
    playerIdle,

    enemy,

    count
};

class AnimationHandle
{
public:

    static void Initialize();
    static const Animation& Get(AnimationID id);

private:
    static std::array<Animation, static_cast<size_t>(AnimationID::count)> animations_;
    static bool initialized_;

    static constexpr std::array<std::pair<AnimationID, const char*>, static_cast<size_t>(AnimationID::count)> animationDefinitions_ =
    {
        {
            // 基本的なアニメーション
            { AnimationID::player,     "Resources/models/player/player.gltf" },
            { AnimationID::playerIdle,     "Resources/models/player/player.gltf" },

            { AnimationID::enemy ,  "Resources/models/enemy/zombi.gltf" },
        }
    };
};
