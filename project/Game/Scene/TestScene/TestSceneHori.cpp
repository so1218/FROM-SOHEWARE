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
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = objectManager_.Create<FollowCamera>(engine_, camera_.get(), player_);
    objectManager_.Create<Grid>(engine_);

    player_->SetFollowCamera(followCamera_);
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