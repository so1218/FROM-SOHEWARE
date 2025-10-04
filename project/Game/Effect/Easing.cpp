#define _USE_MATH_DEFINES
#include <cmath>

#include "Easing.h"

// コンストラクタ
Easing::Easing()
    : easingFunc(nullptr), easingType_(EasingType::EaseInOutSine) {
}

// デストラクタ
Easing::~Easing() {};

// イージング関数を設定
void Easing::SetEasing(EasingType easingType)
{
    this->easingType_ = easingType;
    this->easingFunc = GetEasingFunction(easingType);
}

// イージング関数を取得
float (*Easing::GetEasingFunction(EasingType easingType))(const float&)
{
    switch (easingType)
    {
    case EasingType::EaseLinear: return &Easing::EaseLinear;
    case EasingType::EaseInSine: return &Easing::EaseInSine;
    case EasingType::EaseOutSine: return &Easing::EaseOutSine;
    case EasingType::EaseInOutSine: return &Easing::EaseInOutSine;
    case EasingType::EaseInQuad: return &Easing::EaseInQuad;
    case EasingType::EaseOutQuad: return &Easing::EaseOutQuad;
    case EasingType::EaseInOutQuad: return &Easing::EaseInOutQuad;
    case EasingType::EaseInCubic: return &Easing::EaseInCubic;
    case EasingType::EaseOutCubic: return &Easing::EaseOutCubic;
    case EasingType::EaseInOutCubic: return &Easing::EaseInOutCubic;
    case EasingType::EaseInQuart: return &Easing::EaseInQuart;
    case EasingType::EaseOutQuart: return &Easing::EaseOutQuart;
    case EasingType::EaseInOutQuart: return &Easing::EaseInOutQuart;
    case EasingType::EaseInExpo: return &Easing::EaseInExpo;
    case EasingType::EaseOutExpo: return &Easing::EaseOutExpo;
    case EasingType::EaseInOutExpo: return &Easing::EaseInOutExpo;
    case EasingType::EaseInCirc: return &Easing::EaseInCirc;
    case EasingType::EaseOutCirc: return &Easing::EaseOutCirc;
    case EasingType::EaseInOutCirc: return &Easing::EaseInOutCirc;
    case EasingType::EaseInBack: return &Easing::EaseInBack;
    case EasingType::EaseOutBack: return &Easing::EaseOutBack;
    case EasingType::EaseInOutBack: return &Easing::EaseInOutBack;
    case EasingType::EaseInElastic: return &Easing::EaseInElastic;
    case EasingType::EaseOutElastic: return &Easing::EaseOutElastic;
    case EasingType::EaseInOutElastic: return &Easing::EaseInOutElastic;
    case EasingType::EaseInBounce: return &Easing::EaseInBounce;
    case EasingType::EaseOutBounce: return &Easing::EaseOutBounce;
    case EasingType::EaseInOutBounce: return &Easing::EaseInOutBounce;
    default: return &Easing::EaseInOutSine;
    }
}

// イージングの初期化
void Easing::InitEasing()
{
    interval_ = 0.01f;
    cycle_ = 0.0f;
    timer_ = 0.0f;
    easeTimer_ = 0.0f;
    isEase_ = false;
}

// 汎用線形補間（テンプレート）
//template <typename T>
void Easing::CountEaseLinear(int start, int end, int& current)
{
    if (timer_ >= 1.0f) {
        current = end;
        timer_ = 0.0f;
        isEase_ = false;
    }
    else
    {
        timer_ += interval_;
        easeTimer_ = easingFunc(timer_);
        current = static_cast<int>((1.0f - easeTimer_) * static_cast<int>(start) + easeTimer_ * static_cast<int>(end));
    }
}

void Easing::CountEaseLinear(float start, float end, float& current)
{

    if (timer_ >= 1.0f) {
        current = end;
        timer_ = 0.0f;
        isEase_ = false;
    }
    else
    {
        timer_ += interval_;
        easeTimer_ = easingFunc(timer_);
        current = static_cast<float>((1.0f - easeTimer_) * static_cast<float>(start) + easeTimer_ * static_cast<float>(end));
    }
}

// unsigned int 用の線形補間
void Easing::CountEaseLinear(unsigned int start, unsigned int end, unsigned int& current)
{
    unsigned int startR = (start >> 24) & 0xff;
    unsigned int startG = (start >> 16) & 0xff;
    unsigned int startB = (start >> 8) & 0xff;
    unsigned int startA = start & 0xff;

    unsigned int endR = (end >> 24) & 0xff;
    unsigned int endG = (end >> 16) & 0xff;
    unsigned int endB = (end >> 8) & 0xff;
    unsigned int endA = end & 0xff;

    unsigned int newR, newG, newB, newA;
    if (timer_ >= 1.0f) {
        newR = endR;
        newG = endG;
        newB = endB;
        newA = endA;
        isEase_ = false;
    }
    else {
        timer_ += interval_;
        if (timer_ >= 1.0f) {
            timer_ = 1.0f;
        }
        easeTimer_ = easingFunc(timer_);
        newR = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startR) + easeTimer_ * static_cast<float>(endR));
        newG = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startG) + easeTimer_ * static_cast<float>(endG));
        newB = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startB) + easeTimer_ * static_cast<float>(endB));
        newA = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startA) + easeTimer_ * static_cast<float>(endA));
    }

    current = (newR << 24) | (newG << 16) | (newB << 8) | newA;
}

// Vector3 用の線形補間
void Easing::CountEaseLinear(Vector3 start, Vector3 end, Vector3& current)
{

    if (timer_ >= 1.0f) {
        current = end;
        easeTimer_ = 0.0f;
        timer_ = 0.0f;
        isEase_ = false;

    }
    else {
        timer_ += interval_;
        easeTimer_ = easingFunc(timer_);
        current.x = (1.0f - easeTimer_) * start.x + easeTimer_ * end.x;
        current.y = (1.0f - easeTimer_) * start.y + easeTimer_ * end.y;
        current.z = (1.0f - easeTimer_) * start.z + easeTimer_ * end.z;
    }
}

void Easing::ReverseEaseLinear(float start, float end, float& current)
{
    if (isReverse_) {
        // start -> end への遷移
        if (timer_ <= 0.0f) {
            current = start;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = false;  // 逆方向への遷移を開始
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            current = (1.0f - easeTimer_) * start + easeTimer_ * end;
        }
    }
    else {
        // end -> start への逆方向の遷移
        if (timer_ <= 0.0f) {
            current = end;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = true;  // 元の方向に戻る
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            current = (1.0f - easeTimer_) * end + easeTimer_ * start;
        }
    }
}

void Easing::ReverseEaseLinear(Vector3 start, Vector3 end, Vector3& current)
{

    if (isReverse_) {
        // start -> end への遷移
        if (timer_ <= 0.0f) {
            current = start;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = false;  // 逆方向への遷移を開始
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            current.x = (1.0f - easeTimer_) * start.x + easeTimer_ * end.x;
            current.y = (1.0f - easeTimer_) * start.y + easeTimer_ * end.y;
            current.z = (1.0f - easeTimer_) * start.z + easeTimer_ * end.z;
        }
    }
    else {
        // end -> start への逆方向の遷移
        if (timer_ <= 0.0f) {
            current = end;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = true;  // 元の方向に戻る
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            current.x = (1.0f - easeTimer_) * end.x + easeTimer_ * start.x;
            current.y = (1.0f - easeTimer_) * end.y + easeTimer_ * start.y;
            current.z = (1.0f - easeTimer_) * end.z + easeTimer_ * start.z;
        }
    }
}

void Easing::OnceReverseEaseLinear(Vector3 start, Vector3 end, Vector3& current)
{
    if (isReverse_) {
        // start -> end
        if (timer_ <= 0.0f) {
            current = start;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = false;  // 再び逆方向へ
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
                current = start;
                isEase_ = false;
            }
            easeTimer_ = easingFunc(timer_);
            current.x = (1.0f - easeTimer_) * start.x + easeTimer_ * end.x;
            current.y = (1.0f - easeTimer_) * start.y + easeTimer_ * end.y;
            current.z = (1.0f - easeTimer_) * start.z + easeTimer_ * end.z;
        }
    }
    else {
        // end -> start（逆方向）
        if (timer_ <= 0.0f) {
            current = end;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = true;
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
                current = end;
            }
            easeTimer_ = easingFunc(timer_);
            current.x = (1.0f - easeTimer_) * end.x + easeTimer_ * start.x;
            current.y = (1.0f - easeTimer_) * end.y + easeTimer_ * start.y;
            current.z = (1.0f - easeTimer_) * end.z + easeTimer_ * start.z;
        }
    }
}

// 色遷移のイージング関数
void Easing::ReverseColorLinear(unsigned int start, unsigned int end, unsigned int& current)
{
    unsigned int startR = (start >> 24) & 0xff;
    unsigned int startG = (start >> 16) & 0xff;
    unsigned int startB = (start >> 8) & 0xff;
    unsigned int startA = start & 0xff;

    unsigned int endR = (end >> 24) & 0xff;
    unsigned int endG = (end >> 16) & 0xff;
    unsigned int endB = (end >> 8) & 0xff;
    unsigned int endA = end & 0xff;

    unsigned int newR, newG, newB, newA;

    if (isReverse_) {
        // start -> end への遷移
        if (timer_ <= 0.0f) {
            newR = startR;
            newG = startG;
            newB = startB;
            newA = startA;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = false;

        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            newR = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startR) + easeTimer_ * static_cast<float>(endR));
            newG = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startG) + easeTimer_ * static_cast<float>(endG));
            newB = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startB) + easeTimer_ * static_cast<float>(endB));
            newA = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startA) + easeTimer_ * static_cast<float>(endA));
        }
    }
    else {
        // end -> start への逆方向の遷移
        if (timer_ <= 0.0f) {
            newR = endR;
            newG = endG;
            newB = endB;
            newA = endA;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = true;
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            newR = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endR) + easeTimer_ * static_cast<float>(startR));
            newG = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endG) + easeTimer_ * static_cast<float>(startG));
            newB = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endB) + easeTimer_ * static_cast<float>(startB));
            newA = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endA) + easeTimer_ * static_cast<float>(startA));
        }
    }

    current = (newR << 24) | (newG << 16) | (newB << 8) | newA;
}

void Easing::OnceReverseColorLinear(unsigned int start, unsigned int end, unsigned int& current)
{
    unsigned int startR = (start >> 24) & 0xff;
    unsigned int startG = (start >> 16) & 0xff;
    unsigned int startB = (start >> 8) & 0xff;
    unsigned int startA = start & 0xff;

    unsigned int endR = (end >> 24) & 0xff;
    unsigned int endG = (end >> 16) & 0xff;
    unsigned int endB = (end >> 8) & 0xff;
    unsigned int endA = end & 0xff;

    unsigned int newR, newG, newB, newA;

    if (isReverse_) {
        // start -> end への遷移
        if (timer_ <= 0.0f) {
            newR = startR;
            newG = startG;
            newB = startB;
            newA = startA;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = false;
            if (hasReverse_)
            {
                isEase_ = false;
            }
            hasReverse_ = false;
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            newR = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startR) + easeTimer_ * static_cast<float>(endR));
            newG = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startG) + easeTimer_ * static_cast<float>(endG));
            newB = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startB) + easeTimer_ * static_cast<float>(endB));
            newA = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(startA) + easeTimer_ * static_cast<float>(endA));
        }
    }
    else {
        // end -> start への逆方向の遷移
        if (timer_ <= 0.0f) {
            newR = endR;
            newG = endG;
            newB = endB;
            newA = endA;
            easeTimer_ = 0.0f;
            timer_ = 1.0f;
            isReverse_ = true;
            hasReverse_ = true;
        }
        else {
            timer_ -= interval_;
            if (timer_ <= 0.0f) {
                timer_ = 0.0f;
            }
            easeTimer_ = easingFunc(timer_);
            newR = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endR) + easeTimer_ * static_cast<float>(startR));
            newG = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endG) + easeTimer_ * static_cast<float>(startG));
            newB = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endB) + easeTimer_ * static_cast<float>(startB));
            newA = static_cast<unsigned int>((1.0f - easeTimer_) * static_cast<float>(endA) + easeTimer_ * static_cast<float>(startA));
        }
    }

    current = (newR << 24) | (newG << 16) | (newB << 8) | newA;
}

void Easing::SetEaseDurationFrames(int frames)
{
    if (frames > 0)
    {
        interval_ = 1.0f / static_cast<float>(frames);
    }
}

// イージングの関数群
float Easing::EaseLinear(const float& t) {
    return t;
}
float Easing::EaseInSine(const float& t) {
    return 1 - cosf((t * (float)M_PI) / 2);
}
float Easing::EaseOutSine(const float& t) {
    return sinf((t * (float)M_PI) / 2);
}
float Easing::EaseInOutSine(const float& t) {
    return 0.5f * (1 - cosf((float)M_PI * t));
}
float Easing::EaseInQuad(const float& t) {
    return t * t;
}
float Easing::EaseOutQuad(const float& t) {
    return -t * (t - 2);
}
float Easing::EaseInOutQuad(const float& t) {
    if (t < 0.5f) {
        return 2.0f * t * t;
    }
    else {
        return -2.0f * t * (t - 2.0f) - 1.0f;
    }
}
float Easing::EaseInCubic(const float& t) {
    return t * t * t;
}
float Easing::EaseOutCubic(const float& t) {
    float t_ = t - 1;
    return 1 + t_ * t_ * t_;
}
float Easing::EaseInOutCubic(const float& t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    else {
        float t_ = t - 1;
        return 1 + 4.0f * t_ * t_ * t_;
    }
}
float Easing::EaseInQuart(const float& t) {
    return t * t * t * t;
}
float Easing::EaseOutQuart(const float& t) {
    float t_ = t - 1;
    return 1 - t_ * t_ * t_ * t_;
}
float Easing::EaseInOutQuart(const float& t) {
    if (t < 0.5f) {
        return 8.0f * t * t * t * t;
    }
    else {
        float t_ = t - 1;
        return 1 - 8.0f * t_ * t_ * t_ * t_;
    }
}
float Easing::EaseInExpo(const float& t) {
    return t == 0.0f ? 0.0f : powf(2, 10 * (t - 1));
}
float Easing::EaseOutExpo(const float& t) {
    return t == 1.0f ? 1.0f : 1 - powf(2, -10 * t);
}
float Easing::EaseInOutExpo(const float& t) {
    if (t == 0.0f || t == 1.0f) {
        return t;
    }
    float tt = t * 2;
    if (tt < 1) {
        return 0.5f * powf(2, 10 * (tt - 1));
    }
    tt -= 1;
    return 0.5f * (2 - powf(2, -10 * tt));
}
float Easing::EaseInCirc(const float& t) {
    return 1 - sqrtf(1 - t * t);
}
float Easing::EaseOutCirc(const float& t) {
    return sqrtf(1 - (t - 1) * (t - 1));
}
float Easing::EaseInOutCirc(const float& t) {
    if (t < 0.5f) {
        return 0.5f * (1 - sqrtf(1 - t * t * 4));
    }
    else {
        float tt = t - 1;
        return 0.5f * (sqrtf(1 - tt * tt * 4) + 1);
    }
}
float Easing::EaseInBack(const float& t) {
    const float s = 1.70158f;
    return t * t * ((s + 1) * t - s);
}
float Easing::EaseOutBack(const float& t) {
    const float s1 = 6.00158f;
    const float s2 = s1 + 1;
    return 1 + s2 * powf(t - 1, 3) + s1 * powf(t - 1, 2);
}
float Easing::EaseInOutBack(const float& t) {
    const float s1 = 6.00158f;
    const float s2 = s1 + 1;
    if (t < 0.5f) {
        return 0.5f * (t * t * ((s2 + 1) * t - s2));
    }
    else {
        return 0.5f * (t * t * ((s2 + 1) * t - s2) + 2);
    }
}
float Easing::EaseInElastic(const float& t) {
    const float p = 0.3f;
    const float s = p / 4.0f;
    if (t == 0.0f || t == 1.0f) return t;
    return -powf(2, 10 * (t - 1)) * sinf((t - 1 - s) * (2 * (float)M_PI) / p);
}
float Easing::EaseOutElastic(const float& t) {
    const float p = 0.3f;
    const float s = p / 4.0f;
    if (t == 0.0f || t == 1.0f) return t;
    return powf(2, -10 * t) * sinf((t - s) * (2 * (float)M_PI) / p) + 1;
}
float Easing::EaseInOutElastic(const float& t) {
    const float p = 0.45f;
    const float s = p / 4.0f;
    if (t == 0.0f || t == 1.0f) return t;
    float tt = t * 2;
    if (tt < 1) {
        return -0.5f * powf(2, 10 * (tt - 1)) * sinf((tt - 1 - s) * (2 * (float)M_PI) / p);
    }
    tt -= 1;
    return powf(2, -10 * tt) * sinf((tt - s) * (2 * (float)M_PI) / p) * 0.5f + 1;
}
float Easing::EaseInBounce(const float& t) {
    return 1.0f - EaseOutBounce(1.0f - t);
}
float Easing::EaseOutBounce(const float& t) {
    if (t < (1 / 2.75f)) {
        return 7.5625f * t * t;
    }
    else if (t < (2 / 2.75f)) {
        float t_ = t - (1.5f / 2.75f);
        return 7.5625f * t_ * t_ + 0.75f;
    }
    else if (t < (2.5 / 2.75f)) {
        float t_ = t - (2.25f / 2.75f);
        return 7.5625f * t_ * t_ + 0.9375f;
    }
    else {
        float t_ = t - (2.625f / 2.75f);
        return 7.5625f * t_ * t_ + 0.984375f;
    }
}
float Easing::EaseInOutBounce(const float& t) {
    if (t < 0.5f) {
        return 0.5f * EaseInBounce(t * 2.0f);
    }
    else {
        return 0.5f * EaseOutBounce(t * 2.0f - 1.0f) + 0.5f;
    }
}