#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

// タイトルの文字とSTART GAME
class TitleMenu : public FS::GameObject
{
public:
    TitleMenu(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

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
        FadeIn,    // 黒い画面が明けていく
        Waiting,   // 決定を待つ
        Started,
    };

    bool IsConfirmTriggered() const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> startText_;
    std::unique_ptr<FS::Sprite> blackScreen_;

    Phase phase_ = Phase::FadeIn;
    float phaseTimer_ = 0.0f;
    bool isInputEnabled_ = true;

    // パラメータ
    float fadeInTime_ = 2.0f;       // 黒い画面が明けるまで
    float textFadeInDelay_ = 1.0f;  // 画面が明け始めてから文字が出始めるまで
    float textFadeTime_ = 1.0f;
    float fadeOutTime_ = 0.6f;      // 決定してから文字が消えるまで
    float blinkSpeed_ = 2.5f;       // START GAME の点滅の速さ
    float blinkMinOpacity_ = 0.35f;
};
