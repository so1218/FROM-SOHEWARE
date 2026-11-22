#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class GameOverScreen : public FS::GameObject
{
public:
    explicit GameOverScreen(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // 倒れた瞬間に呼ぶ
    void Begin();
    // 途中でやめて何も出していない状態に戻す
    void Cancel();

    // 画面が真っ暗になった瞬間に一度だけ true を返す
    bool TakeRestartRequest();
    // 倒れてから画面が明けきるまで
    bool IsActive() const { return phase_ != Phase::Hidden; }

private:
    enum class Phase
    {
        Hidden,
        Dying,      
        Revealed,   
        FadeOut,    
        Hold,       
        FadeIn,     
    };

    void ChangePhase(Phase phase);
    void UpdateTimeScale();
    void ApplyVignette();
    void DrawDeathText(float opacity);
    void DrawPrompt(float opacity);
    void PlayIfLoaded(const std::string& name, float volume);

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    std::unique_ptr<FS::Sprite> dimOverlay_;
    std::unique_ptr<FS::Sprite> blackout_;
    std::unique_ptr<FS::Text> deathText_;
    std::unique_ptr<FS::Text> echoText_;
    std::unique_ptr<FS::Sprite> keyIcon_;
    std::unique_ptr<FS::Sprite> buttonIcon_;
    std::unique_ptr<FS::Text> separatorText_;
    std::unique_ptr<FS::Text> promptText_;

    Phase phase_ = Phase::Hidden;
    float phaseTimer_ = 0.0f;
    float deathTimer_ = 0.0f;    
    float revealTimer_ = 0.0f;    
    bool isRestartRequested_ = false;
    bool isSlowMotionActive_ = false;
    float timeScaleBeforeDeath_ = 1.0f;

    // 倒れた直後
    float slowMotionScale_ = 0.3f;
    float slowMotionTime_ = 1.2f;
    FS::Vector4 dimColor_ = { 0.0f, 0.0f, 0.0f, 0.55f };
    float dimTime_ = 1.5f;
    float vignetteAmount_ = 0.8f;
    FS::Vector3 vignetteColor_ = { 0.12f, 0.0f, 0.0f };

    // Death
    float deathTextDelay_ = 1.8f;
    float deathTextFontSize_ = 150.0f;
    float deathTextOffsetY_ = -20.0f;
    FS::Vector4 deathTextColor_ = { 0.72f, 0.04f, 0.03f, 1.0f };
    FS::Vector4 deathTextGlowColor_ = { 0.45f, 0.0f, 0.0f, 0.8f };
    float deathTextGlowSoftness_ = 12.0f;
    float deathTextStartScale_ = 1.35f;
    float deathTextSettleTime_ = 0.6f;
    float deathTextFadeTime_ = 0.3f;
    float deathTextDrift_ = 0.04f;
    float deathTextDriftTime_ = 6.0f;
    float echoScale_ = 1.9f;
    float echoTime_ = 1.1f;
    float echoOpacity_ = 0.45f;
    float deathTextSoundVolume_ = 0.7f;

    // 再開の案内
    float promptDelay_ = 1.4f;
    float promptFadeTime_ = 0.5f;
    float promptOffsetY_ = 210.0f;
    float promptFontSize_ = 24.0f;
    float promptIconSize_ = 34.0f;
    float promptSpacing_ = 8.0f;
    FS::Vector4 promptColor_ = { 1.0f, 1.0f, 1.0f, 0.85f };

    // リスタート
    float fadeOutTime_ = 0.9f;
    float holdTime_ = 0.5f;
    float fadeInTime_ = 1.0f;
};
