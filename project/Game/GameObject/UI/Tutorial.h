#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

class Player;

class Tutorial : public FS::GameObject
{
public:
    Tutorial(FS::Engine* engine, Player* player);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // まだ終えていない説明から始める
    void Begin();
    void Stop() { StartStep(Step::Finished); }
    // 最後の説明まで終えたか
    bool IsCompleted() const { return !isEnabled_ || resumeStep_ == Step::Finished; }
    // 長押しで飛ばしたか
    bool IsSkipped() const { return isSkipped_; }

private:
    // 説明の順番
    enum class Step
    {
        Move,
        Sprint,
        Dodge,
        Jump,
        Crawl,
        Hide,         
        Aim,
        Shoot,
        Reload,
        SwitchAmmo,
        Ignite,      
        Complete,
        Finished,
    };

    // 1 つの説明の表示の流れ
    enum class Phase
    {
        Delay,     
        FadeIn,
        Waiting,  
        Cleared,  
        FadeOut,
    };

    // 説明 1 つ分の文章と、キー・ボタンの画像
    struct StepContent
    {
        std::string saveKey;   
        std::string message;
        std::string keyboardIcons;
        std::string controllerIcons;
    };

    bool IsStepCleared();
    void StartStep(Step step);
    void UpdateInputDevice();
    void ApplyIcons();
    void CreatePanelTexture();
    // 長押しで飛ばす
    bool UpdateSkip(float deltaTime);
    void ApplySkipIcon();
    void DrawSkipHint();

    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;

    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Sprite> panel_;
    std::unique_ptr<FS::Text> messageText_;
    std::vector<std::unique_ptr<FS::Sprite>> iconSprites_;
    size_t iconCount_ = 0;

    // 長押しで飛ばす案内
    std::unique_ptr<FS::Sprite> skipIcon_;
    std::unique_ptr<FS::Text> skipText_;
    std::unique_ptr<FS::Sprite> skipBarBack_;
    std::unique_ptr<FS::Sprite> skipBarFill_;
    bool hasSkipIcon_ = false;
    float skipHoldTimer_ = 0.0f;
    float skipHintVisibility_ = 0.0f;
    bool isSkipped_ = false;

    std::vector<StepContent> stepContents_;
    Step currentStep_ = Step::Move;
    // 次に始める説明
    Step resumeStep_ = Step::Move;
    Phase phase_ = Phase::Delay;
    float phaseTimer_ = 0.0f;
    float visibility_ = 0.0f;      
    float heldTime_ = 0.0f;       
    int shotCountAtStart_ = 0;
    int reloadCountAtStart_ = 0;
    int igniteCountAtStart_ = 0;
    int ammoTypeAtStart_ = 0;
    bool isUsingController_ = false;

    // パラメータ
    bool isEnabled_ = true;
    float startDelay_ = 1.0f;      
    float fadeTime_ = 0.3f;
    float clearedWaitTime_ = 0.6f;  
    float requiredHoldTime_ = 1.0f; 
    float completeShowTime_ = 2.0f; 
    float skipHoldTime_ = 1.0f;
    FS::Vector2 skipHintMargin_ = { 40.0f, 40.0f };
    float skipHintFontSize_ = 22.0f;
    float skipHintIconSize_ = 36.0f;
    float skipHintSpacing_ = 8.0f;
    FS::Vector4 skipHintColor_ = { 1.0f, 1.0f, 1.0f, 0.8f };
    float skipBarHeight_ = 4.0f;
    float skipBarOffsetY_ = 26.0f;
    FS::Vector4 skipBarBackColor_ = { 1.0f, 1.0f, 1.0f, 0.2f };
    FS::Vector4 skipBarFillColor_ = { 1.0f, 1.0f, 1.0f, 0.9f };

    FS::Vector2 panelOffset_ = { 0.0f, 200.0f };
    FS::Vector2 panelSize_ = { 1280.0f, 130.0f };
    FS::Vector4 panelColor_ = { 0.0f, 0.0f, 0.0f, 0.7f };
    float panelEdgeFade_ = 0.35f;   

    FS::Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fontSize_ = 28.0f;
    float textOffsetY_ = -24.0f;   
    float iconSize_ = 48.0f;
    float iconSpacing_ = 8.0f;
    float iconOffsetY_ = 26.0f;     
};
