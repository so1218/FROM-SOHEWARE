#pragma once

#include "Engine.h"

// アニメーションID
enum class AnimationID
{
    // 基本的なアニメーション
    sneakWalk,
    walk,
    ryu,
    shrimp,

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
            { AnimationID::sneakWalk,       "Resources/models/animated/sneakWalk2.gltf" },
            { AnimationID::walk,       "Resources/models/animated/walk.gltf" },
            { AnimationID::ryu,     "Resources/models/animated/animatedRyu.gltf" },
            { AnimationID::shrimp,     "Resources/models/shrimp/ShrimpTailFripAnimation.gltf" },
        }
    };
};
