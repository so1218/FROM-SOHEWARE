#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class TitleMenu : public FS::GameObject
{
public:
    TitleMenu(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    bool HasStarted() const { return phase_ == Phase::Started; }
    // 何も出さずに開始済みにする
    void Skip();
    // 最初の黒い画面からやり直す
    void Restart();
    // ポーズメニューを開いている間は決定を受け付けない
    void SetInputEnabled(bool isEnabled) { isInputEnabled_ = isEnabled; }

private:
    enum class Phase
    {
        FadeIn,   
        Waiting,  
        Started,
    };

    bool IsConfirmTriggered() const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> subtitleText_;
    std::unique_ptr<FS::Text> startText_;
    std::unique_ptr<FS::Sprite> blackScreen_;

    Phase phase_ = Phase::FadeIn;
    float phaseTimer_ = 0.0f;
    bool isInputEnabled_ = true;

    // パラメータ
    float fadeInTime_ = 2.0f;       
    float textFadeInDelay_ = 1.0f;  
    float textFadeTime_ = 1.0f;
    float fadeOutTime_ = 0.6f;      
    float blinkSpeed_ = 2.5f;       
    float blinkMinOpacity_ = 0.35f;
};
