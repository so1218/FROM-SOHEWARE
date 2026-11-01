#pragma once
#include "BaseScene.h"
#include "FollowCamera.h"
#include "TitleCamera.h"

class Player;
class Tutorial;
class TitleMenu;
class GameUI;
class EnemyManager;

// タイトルからゲームまで同じシーンで行う。
// タイトル画面 → START GAME でカメラがプレイヤーへ寄る → 寄りきったら操作とチュートリアルを始める
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

    std::unique_ptr<FollowCamera> followCamera_;
    std::unique_ptr<TitleCamera> titleCamera_;
    Player* player_ = nullptr;
    Tutorial* tutorial_ = nullptr;
    TitleMenu* titleMenu_ = nullptr;
    GameUI* gameUI_ = nullptr;
    EnemyManager* enemyManager_ = nullptr;

    Phase phase_ = Phase::Title;
};
