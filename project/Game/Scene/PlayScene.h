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
class GameOverScreen;
class CoinManager;

class PlayScene : public FS::BaseScene
{
public:
    PlayScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
    void OnUpdate() override;
    void OnDraw() override;
    void OnDebugGui() override;
    void OnFinalize() override;

private:
    enum class Phase
    {
        Title,
        CameraMove,
        Playing,
        GameOver,   
    };

    void EnterTitle();
    void StartPlaying();
    void ChangePhase(Phase phase);
    // フェーズごとの設定
    void ApplyPhaseSettings();
    // 状態に戻してタイトルへ
    void ReturnToTitle();
    // 倒れたあと、初期位置から遊び直す
    void RestartFromSpawn();
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
    GameOverScreen* gameOverScreen_ = nullptr;
    CoinManager* coinManager_ = nullptr;

    Phase phase_ = Phase::Title;

    bool isPaused_ = false;
    float timeScaleBeforePause_ = 1.0f;
};
