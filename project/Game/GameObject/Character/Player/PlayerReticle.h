#pragma once
#include "PropertyBinder.h"
#include "Sprite.h"

class PlayerReticle
{
public:
    struct Config
    {
        float lineThickness = 2.0f;     // 線の太さ
        float lineLength = 12.0f;       // 線の長さ
        float maxGap = 35.0f;           // 最大拡散距離
        float minGap = 5.0f;            // 最小収束距離
        float centerDotSize = 3.0f;     // 完全収束時の中心点サイズ

        float focusTime = 1.0f;         // フォーカス完了時間(秒)
        float expandSpeed = 8.0f;       // 移動・射撃時の拡散スピード

        float threshold = 0.8f;      // 切り替わり地点
        float midRatio = 0.35f;      // thresholdでの収束割合
        float finalExponent = 3.0f;  // threshold以降の加速の強さ
    };

    PlayerReticle(FE::Engine* engine);
    ~PlayerReticle() = default;

    void Initialize();
    void Update(bool isMoving, bool isAiming);
    void Draw();
    void DebugDraw();

    void OnShootRecoil();
    void Reset();

    // フォーカス率（0.0 ~ 1.0）を取得（武器の威力・ブレ角計算用）
    float GetFocusRatio() const { return focusRatio_; }

private:
    FE::Engine* engine_ = nullptr;
    std::unique_ptr<FE::Sprite> sprite_;
    std::unique_ptr<FE::PropertyBinder> binder_;

    Config config_;

    float focusTimer_ = 0.0f;
    float focusRatio_ = 0.0f;
    float reticleAlpha_ = 0.0f;
};