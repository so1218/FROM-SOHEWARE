#include "TestSceneHori.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "SceneManager.h"
#include "AudioPlayer.h"

using namespace MyFrom;

TestSceneHori::TestSceneHori(Engine* engine)
    : BaseScene(engine)
{
    // インスタンスを作成
    auto player = std::make_unique<Player>(engine_, camera_.get());
    player_ = player.get();

    auto followCamera = std::make_unique<FollowCamera>(engine_, camera_.get(), player_);
    followCamera_ = followCamera.get();
    player_->SetFollowCamera(followCamera_);
  
    auto grid = std::make_unique<Grid>(engine_);

    // 作成したゲームオブジェクトを管理クラスに登録
    objectManager_.AddObject(std::move(player));
    objectManager_.AddObject(std::move(followCamera));
    objectManager_.AddObject(std::move(grid));
}

void TestSceneHori::OnInitialize()
{
    // ライトの設定
    engine_->lightManager_->GetDirectionalLightData()[0].enable = true;
    engine_->lightManager_->GetDirectionalLightData()[0].direction = { -0.05f,-1.45f,1.4f };
    engine_->lightManager_->GetDirectionalLightData()[0].intensity = 0.4f;
}

void TestSceneHori::OnUpdate()
{
    AudioPlayer::GetInstance().PlayUnique("titleSceneBGM");
}

void TestSceneHori::OnDraw()
{
   
}

void TestSceneHori::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("ホリシーン");

    ImGui::End();
#endif
}

void TestSceneHori::OnFinalize()
{
}