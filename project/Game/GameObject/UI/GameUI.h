#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "Sprite.h"
#include "Text.h"
#include "PropertyBinder.h"

class GameUI : public FS::GameObject
{
public:
    GameUI(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void SetGameplayVisible(bool isVisible) { isGameplayVisible_ = isVisible; }
    // 時計を一瞬大きく光らせて、制限時間だと気付かせる
    void FlashClock() { clockFlashTimer_ = 0.0f; }
    // 左上の目的と制限時間
    void SetObjective(int startHour, int stormHour, int endHour);
    void SetObjectiveVisible(bool isVisible) { isObjectiveVisible_ = isVisible; }
    // 本番の始まり
    void PlayMissionStart();

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    // テキスト
    std::unique_ptr<FS::Text> timeText_;      
    std::unique_ptr<FS::Text> messageText_;
    std::unique_ptr<FS::Text> objectiveTitleText_;
    std::unique_ptr<FS::Text> objectiveSubText_;
    std::unique_ptr<FS::Sprite> timeBarBack_;
    std::unique_ptr<FS::Sprite> timeBarStorm_;
    std::unique_ptr<FS::Sprite> timeBarFill_;

    // timeText_ の書式
    std::string timeFormat_;
    // 分をこの単位で切り捨てて出す
    int minuteStep_ = 1;

    // 時計を光らせる
    float clockFlashTimer_ = 1000.0f;
    float clockFlashTime_ = 1.2f;
    float clockFlashScale_ = 1.5f;
    FS::Vector4 clockFlashColor_ = { 1.0f, 0.35f, 0.25f, 1.0f };

    // 目的と制限時間
    void DrawObjective();
    bool isObjectiveVisible_ = false;
    float objectiveVisibility_ = 0.0f;
    float objectiveFadeTime_ = 0.6f;
    // 本番の始まりに目的を強調する
    float objectiveIntroTimer_ = 1000.0f;
    float objectiveIntroTime_ = 0.8f;
    float objectiveIntroSlide_ = 60.0f;
    FS::Vector4 objectiveIntroColor_ = { 1.0f, 0.35f, 0.25f, 1.0f };
    std::string missionStartSoundName_ = "missionStart";
    float missionStartSoundVolume_ = 0.8f;
    float startHour_ = 17.0f;
    float stormHour_ = 19.0f;
    float endHour_ = 20.0f;
    FS::Vector2 timeBarScreenAnchor_ = { 0.03f, 0.17f };
    FS::Vector2 timeBarPos_ = { 0.0f, 62.0f };
    FS::Vector2 timeBarSize_ = { 320.0f, 6.0f };
    FS::Vector4 timeBarBackColor_ = { 1.0f, 1.0f, 1.0f, 0.2f };
    FS::Vector4 timeBarStormColor_ = { 0.6f, 0.1f, 0.08f, 0.6f };
    FS::Vector4 timeBarFillColor_ = { 1.0f, 0.95f, 0.85f, 0.9f };

    // まとめて表示/非表示を切り替えるフラグ
    bool isVisible_ = true;
    bool isGameplayVisible_ = true;
};