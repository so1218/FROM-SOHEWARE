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
    // コンストラクタ
    Easing();
    // デストラクタ
    ~Easing();
public:
    // メンバ関数
    // イージング関数の設定
    void SetEasing(EasingType easingType);

    // イージング処理
  /*  template <typename T>*/
    void CountEaseLinear(int start, int end, int& current);
    void CountEaseLinear(float start, float end, float& current);
    void CountEaseLinear(unsigned int start, unsigned int end, unsigned int& current);
    void CountEaseLinear(Vector3 start, Vector3 end, Vector3& curren);
    void ReverseEaseLinear(float start, float end, float& current);
    void ReverseEaseLinear(Vector3 start, Vector3 end, Vector3& current);
    void OnceReverseEaseLinear(Vector3 start, Vector3 end, Vector3& current);
    void OnceReverseColorLinear(unsigned int start, unsigned int end, unsigned int& current);

    // 0.0～1.0の線形な時間 t を受け取り、イージング適用後の時間 (0.0～1.0) を返す
    float Evaluate(const float& t) const;
   
    // イージングの初期化
    void InitEasing();
    // 色遷移のイージング関数
    void ReverseColorLinear(unsigned int start, unsigned int end, unsigned int& current);


public:
    // メンバ変数
    float interval_ = 0.01f;
    float cycle_ = 0.0f;
    float timer_ = 0.0f;
    float easeTimer_ = 0.0f;
    bool isEase_ = false;
    bool isReverse_ = true;
    bool hasReverse_ = false;
    int frameCount_ = 0;
    int runCount_ = 0;
    unsigned int fadeColor_ = 0x00000000;

    // スタートの変数
    unsigned int startColor_ = 0x000000ff;
    int intStartPos_ = 0;
    float floatStartPos_ = 0.0f;
    Vector3 vec3StartPos_ = { 0.0f, 0.0f, 0.0f };

    // エンドの変数
    unsigned int endColor_ = 0x000000ff;
    int intEndPos_ = 0;
    float floatEndPos_ = 0.0f;
    Vector3 vec3EndPos_ = { 0.0f, 0.0f, 0.0f };

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

    // イージング関数を取得
    float (*GetEasingFunction(EasingType easingType))(const float&);
    float (*easingFunc)(const float&);
    EasingType easingType_ = EasingType::EaseInOutSine;
};

