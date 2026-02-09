#include "SampleSceneHori.h"
#include "ImGuiManager.h"
#include "GlobalVariables.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "SceneManager.h"
#include "AudioPlayer.h"
#include "AudioHandle.h"

using namespace FromEngine;

SampleSceneHori::SampleSceneHori(Engine* engine)
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

void SampleSceneHori::OnInitialize()
{
    // ライトの設定
    engine_->lightManager_->GetDirectionalLightData()[0].enable = true;
    engine_->lightManager_->GetDirectionalLightData()[0].direction = { -0.05f,-1.45f,1.4f };
    engine_->lightManager_->GetDirectionalLightData()[0].intensity = 0.4f;
}

void SampleSceneHori::OnUpdate()
{
    AudioPlayer::GetInstance().PlayUnique("titleSceneBGM");
}

void SampleSceneHori::OnDraw()
{
   
}

void SampleSceneHori::OnDebugDraw()
{
#ifdef IS_DEVELOPMENT
    ImGui::Begin("ホリシーン");

    ImGui::End();
#endif
}

void SampleSceneHori::OnFinalize()
{
}