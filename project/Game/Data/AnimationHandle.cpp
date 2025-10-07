#include "AnimationHandle.h"
#include "AnimationLoader.h"

std::array<Animation, static_cast<size_t>(AnimationID::count)> AnimationHandle::animations_{};
bool AnimationHandle::initialized_ = false;

void AnimationHandle::Initialize()
{
    if (initialized_) return;

    for (const auto& def : animationDefinitions_)
    {
        // 配列に登録
        animations_[static_cast<size_t>(def.first)] = LoadAnimationFile(def.second);
    }

    initialized_ = true;
}

const Animation& AnimationHandle::Get(AnimationID id)
{
    assert(initialized_);

    return animations_[static_cast<size_t>(id)];
}