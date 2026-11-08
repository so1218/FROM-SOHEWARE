#pragma once

namespace FS
{

// 押しっぱなしで一定間隔ごとに反応させる
class RepeatInput
{
public:
    // amount は押している量 (0 ~ 1)。押した瞬間と、押し続けている間の一定間隔ごとに true を返す
    bool Update(float amount, float deltaTime)
    {
        if (!isDown_)
        {
            if (amount < kPressThreshold) return false;
            isDown_ = true;
            repeatTimer_ = kFirstRepeatDelay;
            return true;
        }

        // 押したときより低いしきい値で離した扱いにして、スティックが境目で揺れても押し直しにならないようにする
        if (amount < kReleaseThreshold)
        {
            isDown_ = false;
            return false;
        }

        repeatTimer_ -= deltaTime;
        if (repeatTimer_ > 0.0f) return false;
        repeatTimer_ += kRepeatInterval;
        return true;
    }

    void Reset() { isDown_ = false; }

private:
    static constexpr float kPressThreshold = 0.5f;
    static constexpr float kReleaseThreshold = 0.3f;
    static constexpr float kFirstRepeatDelay = 0.4f;
    static constexpr float kRepeatInterval = 0.1f;

    bool isDown_ = false;
    float repeatTimer_ = 0.0f;
};

}
