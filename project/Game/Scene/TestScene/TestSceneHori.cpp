#include "TestSceneHori.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "Ground.h"
#include "GrassField.h"
#include "SceneManager.h"
#include "AudioPlayer.h"

TestSceneHori::TestSceneHori(Engine* engine)
    : BaseScene(engine)
{
	// ゲームオブジェクトの生成・登録
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = objectManager_.Create<FollowCamera>(engine_, camera_.get(), player_);
    //objectManager_.Create<Grid>(engine_);
    objectManager_.Create<Ground>(engine_);
    bubble_ = objectManager_.Create<Bubble>(engine_);
    objectManager_.Create<GrassField>(engine_, player_);

    player_->SetFollowCamera(followCamera_);
}

void TestSceneHori::OnInitialize()
{
    // ライトの設定
    engine_->GetLightManager()->GetDirectionalLightData()[0].enable = true;
    engine_->GetLightManager()->GetDirectionalLightData()[0].direction = { -0.05f,-1.45f,1.4f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].intensity = 0.4f;

    testSceneEmitter_ = engine_->GetParticleSystem()->CreateEmitter("testScene");
    engine_->GetParticleSystem()->AddEmitter(std::move(testSceneEmitter_));
    auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("aura");
    auraEmitter_->SetTargetModel(bubble_->model_.get());
    engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));
}

void TestSceneHori::OnUpdate()
{
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