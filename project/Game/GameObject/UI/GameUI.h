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

private:
    FS::Engine* engine_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    // テキスト
    std::unique_ptr<FS::Text> timeText_;      
    std::unique_ptr<FS::Text> messageText_;
    std::unique_ptr<FS::Text> stageNameText_; 

    // timeText_ の書式
    std::string timeFormat_;
    // 分をこの単位で切り捨てて出す
    int minuteStep_ = 10;

    // まとめて表示/非表示を切り替えるフラグ
    bool isVisible_ = true;
    bool isGameplayVisible_ = true;
};