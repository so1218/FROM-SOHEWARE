#pragma once
#include "BaseScene.h"
#include "FollowCamera.h"
#include "TitleCamera.h"
#include "PropertyBinder.h"

class Player;
class Tutorial;
class TitleMenu;
class GameUI;
class EnemyManager;
class PauseMenu;
class GameOverScreen;
class CoinManager;
class ResultScreen;
class ObjectiveBanner;

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
        Result,
        // 本番の前に目的を見せている間
        Intro,
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
    // 時計を最初の時刻・晴れに戻して止める
    void ResetGameClock();
    // 目的を見せてから本番を始める
    void BeginObjective(bool withFade);
    // チュートリアルを終えていれば目的を見せ、まだならそのまま遊ばせる
    void BeginPlayingOrObjective();
    // 敵に襲わせないフェーズか
    static bool IsEnemyFreePhase(Phase phase) { return phase == Phase::Result || phase == Phase::Intro; }
    // 時計を進め、途中で雷雨にする
    void UpdateGameClock();
    // 経過時間から、いまの時刻を求める
    float CalculateHour(float elapsedTime) const;
    float GetTotalPlayTime() const { return calmDuration_ + stormDuration_; }

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
    ResultScreen* resultScreen_ = nullptr;
    ObjectiveBanner* objectiveBanner_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> flowBinder_;

    Phase phase_ = Phase::Title;

    bool isPaused_ = false;
    float timeScaleBeforePause_ = 1.0f;

    // ゲームの時計
    float playTimer_ = 0.0f;
    bool isClockRunning_ = false;
    bool isStormRequested_ = false;
    bool wasIgnoredByEnemies_ = false;
    float tutorialEndTimer_ = 0.0f;
    bool hasShownObjective_ = false;

    // チュートリアルを終えてから暗転し始めるまで (秒)
    float tutorialEndDelay_ = 2.0f;

    float startHour_ = 17.0f;
    float stormHour_ = 19.0f;
    float endHour_ = 20.0f;
    // 晴れの間の長さ (秒)
    float calmDuration_ = 120.0f;
    // 雷雨の間の長さ (秒)
    float stormDuration_ = 60.0f;
    // 雷雨の時刻ちょうどに降り終わっているよう、何秒前から天候を変え始めるか
    float stormLeadTime_ = 10.0f;
};
