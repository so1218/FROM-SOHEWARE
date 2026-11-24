#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"
#include "RepeatInput.h"

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
    // 黒い画面からの出だしを省いて、決定を待つところから始める
    void SkipFadeIn();
    // 左右の入力でステージを選び直したか
    bool TakeStageChangeRequest();

private:
    enum class Phase
    {
        FadeIn,   
        Waiting,  
        Started,
    };

    bool IsConfirmTriggered() const;
    // 見出しとステージ名と、左右のボタンの絵を出す
    void DrawStageSelect(float opacity);
    // 決定を待つ間の左右の入力で、選ぶステージを変える
    void UpdateStageSelect(float deltaTime);

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Text> titleText_;
    std::unique_ptr<FS::Text> subtitleText_;
    std::unique_ptr<FS::Text> startText_;
    std::unique_ptr<FS::Text> stageText_;
    // ステージ名の上の見出しSTAGE
    std::unique_ptr<FS::Text> stageLabelText_;
    // ステージ名の左右で、選び直せることを見せるボタンの絵
    std::unique_ptr<FS::Sprite> stageLeftIcon_;
    std::unique_ptr<FS::Sprite> stageRightIcon_;
    // ステージごとの名前
    std::vector<std::string> stageNames_;
    float arrowTimer_ = 0.0f;
    std::unique_ptr<FS::Sprite> blackScreen_;
    // 右上のパッド推奨
    std::unique_ptr<FS::Sprite> padHintIcon_;
    std::unique_ptr<FS::Text> padHintText_;

    Phase phase_ = Phase::FadeIn;
    float phaseTimer_ = 0.0f;
    bool isInputEnabled_ = true;
    bool isStageChangeRequested_ = false;
    FS::RepeatInput leftRepeat_;
    FS::RepeatInput rightRepeat_;

    // パラメータ
    float fadeInTime_ = 2.0f;       
    float textFadeInDelay_ = 1.0f;  
    float textFadeTime_ = 1.0f;
    float fadeOutTime_ = 0.6f;      
    float blinkSpeed_ = 2.5f;       
    float blinkMinOpacity_ = 0.35f;
    float stageLabelOpacity_ = 0.6f;      
    float stageArrowSize_ = 36.0f;
    float stageArrowGap_ = 28.0f;          
    float stageArrowBobDistance_ = 5.0f;  
    float stageArrowBobSpeed_ = 3.0f;
};
