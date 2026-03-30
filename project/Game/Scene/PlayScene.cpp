#include "pch.h"
#include "PlayScene.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "ImGuiManager.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "MathUtils.h"
#include "Input.h"
#include "Grid.h"

using namespace FE;

PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine)
{
    // オブジェクトを生成
    auto grid = std::make_unique<Grid>(engine_);

    objectManager_.AddObject(std::move(grid));
}

PlayScene::~PlayScene()
{
}

void PlayScene::OnInitialize()
{
    // 初期化
    camera_->Initialize();

    playSceneEmitter_ = engine_->GetParticleSystem()->CreateEmitter("playScene");
    engine_->GetParticleSystem()->AddEmitter(std::move(playSceneEmitter_));
}

void PlayScene::OnUpdate()
{
    if (Input::GetInstance().IsKeyTriggered(DIK_SPACE) || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonA)
        || Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB))
    {
        // シーンマネージャーを通じてシーン切り替えをリクエスト
        sceneManager_->RequestSceneChange(SceneID::Title);
    }
}

void PlayScene::OnDraw()
{
    
}

void PlayScene::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("プレイシーン");

    ImGui::End();
#endif
}

void PlayScene::OnFinalize()
{
}

