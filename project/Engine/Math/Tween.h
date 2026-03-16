#pragma once
#include "Easing.h"
#include "MathUtils.h"
#include "TimeManager.h"

template <typename T>
class Tween
{
public:
    // アニメーションの開始
    void Start(T* target, T endValue, float duration, EasingType type);

    // 毎フレームの更新
    void Update();

    // アニメーションが完了したか
    bool IsDone() const { return isDone_; }

private:
    T* target_ = nullptr; 
    T startValue_;
    T endValue_;

    float elapsed_ = 0.0f;
    float duration_ = 1.0f; // アニメーションの総時間(秒)
    EasingType type_ = EasingType::EaseLinear;
    bool isDone_ = true;
};

template <typename T>
void Tween<T>::Start(T* target, T endValue, float duration, EasingType type)
{
    target_ = target;
    startValue_ = *target;
    endValue_ = endValue;

    duration_ = (duration <= 0.0f) ? 0.001f : duration; 
    elapsed_ = 0.0f;
    type_ = type;
    isDone_ = false;
}

template <typename T>
void Tween<T>::Update()
{
    if (isDone_ || !target_) return;

    elapsed_ += TimeManager::GetInstance()->GetDeltaTime();
    float t = 1.0f;

    if (elapsed_ >= duration_)
    {
        isDone_ = true;
    }
    else
    {
        t = elapsed_ / duration_;
    }

    // Easing計算とLerpの適用をすべて実行
    float eased_t = Easing::Evaluate(type_, t);
    *target_ = Math::Lerp(startValue_, endValue_, eased_t);
}