#pragma once
#include <optional>
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class ResultScreen : public FS::GameObject
{
public:
    // 結果画面で選べるもの
    enum class Choice
    {
        Retry,
        Title,
    };

    explicit ResultScreen(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 時間切れになった瞬間に呼ぶ
    void Begin(int money);
    // 途中でやめて何も出していない状態に戻す
    void Cancel();

    // 画面が真っ暗になった瞬間に一度だけ、選ばれたものを返す
    std::optional<Choice> TakeRequest();
    bool IsActive() const { return phase_ != Phase::Hidden; }

private:
    enum class Phase
    {
        Hidden,
        Reveal,
        Select,
        FadeOut,
        Hold,
        FadeIn,
    };

    void ChangePhase(Phase phase);
    void UpdateSelection();
    void DrawContent(float opacity);
    void DrawChoices(float opacity);
    void PlayIfLoaded(const std::string& name, float volume);

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> dimOverlay_;
    std::unique_ptr<FS::Sprite> blackout_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> moneyLabelText_;
    std::unique_ptr<FS::Text> moneyText_;
    std::unique_ptr<FS::Text> retryText_;
    std::unique_ptr<FS::Text> titleChoiceText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    float revealTimer_ = 0.0f;
    int money_ = 0;
    Choice selectedChoice_ = Choice::Retry;
    std::optional<Choice> request_;
    bool isHorizontalHeld_ = false;

    // 背景
    FS::Vector4 dimColor_ = { 0.0f, 0.0f, 0.0f, 0.7f };
    float dimTime_ = 1.2f;

    // 見出し
    float titleDelay_ = 0.6f;
    float titleFadeTime_ = 0.8f;
    float titleFontSize_ = 96.0f;
    float titleOffsetY_ = -170.0f;
    FS::Vector4 titleColor_ = { 0.86f, 0.72f, 0.38f, 1.0f };
    FS::Vector4 titleGlowColor_ = { 0.45f, 0.3f, 0.05f, 0.7f };
    float titleGlowSoftness_ = 12.0f;
    float titleCharSpacing_ = 8.0f;
    // 出てからゆっくり大きくなる量
    float titleDrift_ = 0.04f;

    // 所持金
    float moneyDelay_ = 1.4f;
    float moneyCountTime_ = 1.2f;
    float moneyLabelFontSize_ = 28.0f;
    float moneyLabelOffsetY_ = -40.0f;
    float moneyFontSize_ = 80.0f;
    float moneyOffsetY_ = 30.0f;
    FS::Vector4 moneyLabelColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };
    FS::Vector4 moneyColor_ = { 1.0f, 0.85f, 0.4f, 1.0f };
    float moneyCountSoundVolume_ = 0.6f;

    // 選択肢
    float choiceDelay_ = 0.6f;
    float choiceFadeTime_ = 0.4f;
    float choiceFontSize_ = 30.0f;
    float choiceOffsetY_ = 200.0f;
    float choiceSpacing_ = 260.0f;
    FS::Vector4 choiceSelectedColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    FS::Vector4 choiceUnselectedColor_ = { 1.0f, 1.0f, 1.0f, 0.35f };

    // 画面の切り替え
    float fadeOutTime_ = 0.9f;
    float holdTime_ = 0.4f;
    float fadeInTime_ = 1.0f;
};
