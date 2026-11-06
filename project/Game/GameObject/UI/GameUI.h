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

    // タイトル中は遊ぶときの表示 (時間など) を隠す。操作説明 (messageText_) はエディター表示なら出す
    void SetGameplayVisible(bool isVisible) { isGameplayVisible_ = isVisible; }
    void SetRemainingTime(float seconds) { remainingTime_ = seconds; }

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    // テキスト
    std::unique_ptr<FS::Text> instructionText_;     
    std::unique_ptr<FS::Text> timeText_;      
    std::unique_ptr<FS::Text> messageText_;
    std::unique_ptr<FS::Text> stageNameText_;  // 縦書き

    // timeText_ の書式。{} に分、秒の順で入る
    std::string timeFormat_;
    float remainingTime_ = 0.0f;

    // まとめて表示/非表示を切り替えるフラグ
    bool isVisible_ = true;
    bool isGameplayVisible_ = true;
};