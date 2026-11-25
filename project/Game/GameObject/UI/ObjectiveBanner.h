#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class ObjectiveBanner : public FS::GameObject
{
public:
    explicit ObjectiveBanner(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void Begin(bool withFade, bool isFirstTime, int startHour, int endHour);
    void Cancel();

    // 画面が真っ暗になった瞬間に一度だけ true を返す
    bool TakeResetRequest();
    // 目的を出し終えた瞬間に一度だけ true を返す
    bool TakeFinished();
    bool IsActive() const { return phase_ != Phase::Hidden; }

private:
    enum class Phase
    {
        Hidden,
        FadeOut,
        Hold,
        FadeIn,
        Show,
        Leave,
    };

    void ChangePhase(Phase phase);
    void DrawContent(float opacity);

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> blackout_;
    std::unique_ptr<FS::Sprite> band_;
    std::unique_ptr<FS::Sprite> line_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> subText_;
    std::unique_ptr<FS::Text> timeText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    float showDuration_ = 0.0f;
    bool isResetRequested_ = false;
    bool isFinished_ = false;

    // 暗転
    float fadeOutTime_ = 0.8f;
    float holdTime_ = 0.5f;
    float fadeInTime_ = 0.8f;

    // 目的
    float showTime_ = 3.5f;
    float repeatShowTime_ = 1.8f;
    float enterTime_ = 0.6f;
    float leaveTime_ = 0.5f;
    float skipDelay_ = 0.5f;
    FS::Vector4 bandColor_ = { 0.0f, 0.0f, 0.0f, 0.55f };
    float bandHeight_ = 230.0f;
    float lineWidth_ = 520.0f;
    float lineThickness_ = 2.0f;
    FS::Vector4 lineColor_ = { 0.85f, 0.15f, 0.1f, 0.9f };
    float titleFontSize_ = 64.0f;
    float titleOffsetY_ = -40.0f;
    float titleStartScale_ = 1.3f;
    FS::Vector4 titleColor_ = { 0.95f, 0.9f, 0.8f, 1.0f };
    FS::Vector4 titleGlowColor_ = { 0.45f, 0.0f, 0.0f, 0.8f };
    float titleGlowSoftness_ = 10.0f;
    float subFontSize_ = 30.0f;
    float subOffsetY_ = 35.0f;
    FS::Vector4 subColor_ = { 1.0f, 1.0f, 1.0f, 0.9f };
    float timeFontSize_ = 22.0f;
    float timeOffsetY_ = 80.0f;
    FS::Vector4 timeColor_ = { 1.0f, 1.0f, 1.0f, 0.6f };
    float subDelay_ = 0.25f;
};
