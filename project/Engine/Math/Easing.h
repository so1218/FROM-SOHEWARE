#pragma once
#include "Vector.h"

// イージングタイプの列挙クラス
enum class EasingType
{
    EaseLinear,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce
};

class Easing
{
public:
 
    // どのイージングを使うか、進捗はいくつか を渡す
    static float Evaluate(EasingType easingType, const float& t)
    {
        switch (easingType)
        {
            case EasingType::EaseLinear:        return EaseLinear(t);
            case EasingType::EaseInSine:        return EaseInSine(t);
            case EasingType::EaseOutSine:       return EaseOutSine(t);
            case EasingType::EaseInOutSine:     return EaseInOutSine(t);
            case EasingType::EaseInQuad:        return EaseInQuad(t);
            case EasingType::EaseOutQuad:       return EaseOutQuad(t);
            case EasingType::EaseInOutQuad:     return EaseInOutQuad(t);
            case EasingType::EaseInCubic:       return EaseInCubic(t);
            case EasingType::EaseOutCubic:      return EaseOutCubic(t);
            case EasingType::EaseInOutCubic:    return EaseInOutCubic(t);
            case EasingType::EaseInQuart:       return EaseInQuart(t);
            case EasingType::EaseOutQuart:      return EaseOutQuart(t);
            case EasingType::EaseInOutQuart:    return EaseInOutQuart(t);
            case EasingType::EaseInExpo:        return EaseInExpo(t);
            case EasingType::EaseOutExpo:       return EaseOutExpo(t);
            case EasingType::EaseInOutExpo:     return EaseInOutExpo(t);
            case EasingType::EaseInCirc:        return EaseInCirc(t);
            case EasingType::EaseOutCirc:       return EaseOutCirc(t);
            case EasingType::EaseInOutCirc:     return EaseInOutCirc(t);
            case EasingType::EaseInBack:        return EaseInBack(t);
            case EasingType::EaseOutBack:       return EaseOutBack(t);
            case EasingType::EaseInOutBack:     return EaseInOutBack(t);
            case EasingType::EaseInElastic:     return EaseInElastic(t);
            case EasingType::EaseOutElastic:    return EaseOutElastic(t);
            case EasingType::EaseInOutElastic:  return EaseInOutElastic(t);
            case EasingType::EaseInBounce:      return EaseInBounce(t);
            case EasingType::EaseOutBounce:     return EaseOutBounce(t);
            case EasingType::EaseInOutBounce:   return EaseInOutBounce(t);
            default:                            return t;
        }
    }

private:
    // イージング関数
     // 静的メンバ関数として定義
    static float EaseLinear(const float& t);
    static float EaseInSine(const float& t);
    static float EaseOutSine(const float& t);
    static float EaseInOutSine(const float& t);
    static float EaseInQuad(const float& t);
    static float EaseOutQuad(const float& t);
    static float EaseInOutQuad(const float& t);
    static float EaseInCubic(const float& t);
    static float EaseOutCubic(const float& t);
    static float EaseInOutCubic(const float& t);
    static float EaseInQuart(const float& t);
    static float EaseOutQuart(const float& t);
    static float EaseInOutQuart(const float& t);
    static float EaseInExpo(const float& t);
    static float EaseOutExpo(const float& t);
    static float EaseInOutExpo(const float& t);
    static float EaseInCirc(const float& t);
    static float EaseOutCirc(const float& t);
    static float EaseInOutCirc(const float& t);
    static float EaseInBack(const float& t);
    static float EaseOutBack(const float& t);
    static float EaseInOutBack(const float& t);
    static float EaseInElastic(const float& t);
    static float EaseOutElastic(const float& t);
    static float EaseInOutElastic(const float& t);
    static float EaseInBounce(const float& t);
    static float EaseOutBounce(const float& t);
    static float EaseInOutBounce(const float& t);

};

