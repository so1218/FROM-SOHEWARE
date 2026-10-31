#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class Player;

// 操作のチュートリアル。上に説明、下にキーやボタンの画像を出し、その操作ができたら次へ進む
class Tutorial : public FS::GameObject
{
public:
    Tutorial(FS::Engine* engine, Player* player);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

private:
    // 説明の順番
    enum class Step
    {
        Move,
        Sprint,
        Dodge,
        Jump,
        Aim,
        Shoot,
        Complete,
        Finished,
    };

    // 1 つの説明の表示の流れ
    enum class Phase
    {
        Delay,     // 出す前の待ち
        FadeIn,
        Waiting,   // 操作を待っている
        Cleared,   // できたので少し残す
        FadeOut,
    };

    // 説明 1 つ分の文章と、キー・ボタンの画像 (テクスチャ名をカンマ区切り)
    struct StepContent
    {
        std::string message;
        std::string keyboardIcons;
        std::string controllerIcons;
    };

    bool IsStepCleared();
    void StartStep(Step step);
    void UpdateInputDevice();
    void ApplyIcons();
    void CreatePanelTexture();

    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;

    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Sprite> panel_;
    std::unique_ptr<FS::Text> messageText_;
    std::vector<std::unique_ptr<FS::Sprite>> iconSprites_;
    size_t iconCount_ = 0;

    std::vector<StepContent> stepContents_;
    Step currentStep_ = Step::Move;
    Phase phase_ = Phase::Delay;
    float phaseTimer_ = 0.0f;
    float visibility_ = 0.0f;       // 0 = 見えない, 1 = 見える
    float heldTime_ = 0.0f;         // 移動やダッシュを続けている時間
    int shotCountAtStart_ = 0;
    bool isUsingController_ = false;

    // パラメータ
    bool isEnabled_ = true;
    float startDelay_ = 1.0f;       // シーン開始から最初の説明を出すまで
    float fadeTime_ = 0.3f;
    float clearedWaitTime_ = 0.6f;  // できてから消え始めるまで
    float requiredHoldTime_ = 1.0f; // 移動・ダッシュ・エイムを続ける時間
    float completeShowTime_ = 2.0f; // 最後の「完了」を出しておく時間

    FS::Vector2 panelOffset_ = { 0.0f, 200.0f };  // 画面中央からのずれ
    FS::Vector2 panelSize_ = { 1280.0f, 130.0f };
    FS::Vector4 panelColor_ = { 0.0f, 0.0f, 0.0f, 0.7f };
    float panelEdgeFade_ = 0.35f;   // 左右それぞれ、幅のこの割合で透明になっていく

    FS::Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize_ = 28.0f;
    float textOffsetY_ = -24.0f;    // 帯の中心からのずれ
    float iconSize_ = 48.0f;
    float iconSpacing_ = 8.0f;
    float iconOffsetY_ = 26.0f;     // 帯の中心からのずれ
};
