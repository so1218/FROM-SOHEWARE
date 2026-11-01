#pragma once
#include "BaseScene.h"
#include "FollowCamera.h"
#include "TitleCamera.h"

class Player;
class Tutorial;
class TitleMenu;
class GameUI;
class EnemyManager;
class PauseMenu;

class PlayScene : public FS::BaseScene
{
public:
    PlayScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugDraw() override;
    void OnFinalize() override;

private:
    enum class Phase
    {
        Title,
        CameraMove,
        Playing,
    };

    void EnterTitle();
    void StartPlaying();
    // プレイヤー・敵・チュートリアルを最初の状態に戻してタイトルへ
    void ReturnToTitle();
    // ポーズメニューの開け閉めに合わせてゲームを止める・動かす
    void UpdatePauseMenu();

    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<TitleCamera> titleCamera_;
    Player* player_ = nullptr;
    Tutorial* tutorial_ = nullptr;
    TitleMenu* titleMenu_ = nullptr;
    GameUI* gameUI_ = nullptr;
    EnemyManager* enemyManager_ = nullptr;
    PauseMenu* pauseMenu_ = nullptr;

    Phase phase_ = Phase::Title;

    bool isPaused_ = false;
    float timeScaleBeforePause_ = 1.0f;

    float timeLimit_ = 300.0f;      // 秒
    float remainingTime_ = 300.0f;
};
