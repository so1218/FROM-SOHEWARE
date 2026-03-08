#include "pch.h"
#include "Easing.h"

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