#include "pch.h"
#include "TestSceneHori.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "GrassField.h"
#include "SceneManager.h"
#include "AudioPlayer.h"
#include "OrbManager.h"
#include "EnemyManager.h"
#include "EnvironmentPropManager.h"
#include "WeatherEffectManager.h"
#include "PebbleField.h"
#include "FoliageField.h"
#include "GameUI.h"
#include "WaterManager.h"

using namespace FE;

TestSceneHori::TestSceneHori(Engine* engine)
    : BaseScene(engine)
{
	// ゲームオブジェクトの生成・登録
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player_->GetTransform());
    ground_ = objectManager_.Create<Ground>(engine_);
    grassField_ = objectManager_.Create<GrassField>(engine_);
    grassField_->SetTerrain(ground_->GetTerrain());
    treeField_ = objectManager_.Create<TreeField>(engine_);
    treeField_->SetTerrain(ground_->GetTerrain());
    player_->SetTerrain(ground_->GetTerrain());
    bubble_ = objectManager_.Create<Bubble>(engine_);
    objectManager_.Create<WeatherEffectManager>(engine_, camera_.get(), player_, ground_->GetTerrain());
    objectManager_.Create<OrbManager>(engine_, "GameOrb");
    objectManager_.Create<EnemyManager>(engine_, "GameEnemy");
    objectManager_.Create<EnvironmentPropManager>(engine_, "EnvironmentProps");
    objectManager_.Create<PebbleField>(engine_);
    objectManager_.Create<FoliageField>(engine_);
    objectManager_.Create<GameUI>(engine_);
    objectManager_.Create<WaterManager>(engine_, "GameWater");
}

void TestSceneHori::OnInitialize()
{
    // ライトの設定
    engine_->GetLightManager()->GetDirectionalLightData()[0].enable = true;
    engine_->GetLightManager()->GetDirectionalLightData()[0].direction = { 2.6f,-0.4f,1.4f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].color = { 1.0f,193.0f / 255.0f,96.0f / 255.0f,1.0f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].intensity = 0.4f;
    engine_->GetLightManager()->GetDirectionalLightData()[0].volumetricScatteringIntensity = 13.0f;
    engine_->GetPostEffectManager()->GetCombineSettings()->enableVolumetricFog = true;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseIntensity = 0.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->scatteringIntensity = 10.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->extinctionScale = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->ambientLight = { 9.0f / 255.0f,9.0f / 255.0f,9.0f / 255.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->maxDistance = 500.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->extinctionScale = 0.5f;

    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().clear();
    
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().push_back(VolumetricFogPass::FogVolumeData());
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().push_back(VolumetricFogPass::FogVolumeData());
    
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].type = 1;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].position = { 50.0f,22.0f,-50.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].scale = { 50.0f,24.0f,50.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].color = { 24.0f / 255.0f,194.0f / 255.0f,252.0f / 255.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].density = 0.2f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].blendDistance = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].windDirection = { 1.0f,-0.2f,0.7f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].windSpeed = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].coverage = 0.55f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].worleyWeight = 0.95f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].noiseIntensity = 0.9f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].noiseScale = { 0.06f,0.06f,0.06f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].position = { -60.0f,0.0f,60.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].scale.x = 50.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].color = { 120.0f / 255.0f,30.0f / 255.0f,255.0f / 255.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].density = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].blendDistance = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].windDirection = { 1.0f,-0.2f,0.7f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].windSpeed = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].coverage = 0.55f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].worleyWeight = 0.95f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].noiseIntensity = 0.9f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].noiseScale = { 0.06f,0.06f,0.06f };
    engine_->GetPostEffectManager()->GetBrightSettings()->threshold = 0.4f;
    engine_->GetPostEffectManager()->GetBrightSettings()->intensity = 1.1f;
    engine_->GetPostEffectManager()->GetCombineSettings()->enableDoF = true;
    engine_->GetPostEffectManager()->GetDoFSettings()->focusDistance = 45.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->focusRange = 43.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->bokehHighlightIntensity = 3.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->transitionRange = 65.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->bokehRadius = 2.3f;
    engine_->GetPostEffectManager()->GetCombineSettings()->enableSSAO = true;
    engine_->GetPostEffectManager()->GetSSAOSettings()->intensity = 3.5f;
    grassFieldEmitter_ = engine_->GetParticleSystem()->CreateEmitter("grassField");
    grassFieldEmitter_->SetTargetToFollow(&player_->GetTransform());
    engine_->GetParticleSystem()->AddEmitter(std::move(grassFieldEmitter_));

    // 生成・初期化
    auto openingRail = std::make_unique<CameraRail>(engine_, camera_.get(), "HoriScene_Opening");
    openingRail->Initialize();

    // ローカル変数をムーブして CameraManager に渡す
    cameraManager_->AddRail("Opening", std::move(openingRail));

    // デフォルトカメラの設定
    followCamera_->Initialize();
    cameraManager_->ChangeController(followCamera_.get());
}

void TestSceneHori::OnUpdate()
{
    if (Input::GetInstance().IsKeyTriggered(DIK_I))
    {
        // マネージャーに名前を伝える
        cameraManager_->PlayRail("Opening");
    }

}

void TestSceneHori::OnDraw()
{
   
}

void TestSceneHori::OnDebugDraw()
{
#ifdef ENABLE_IMGUI
 
#endif
}

void TestSceneHori::OnFinalize()
{
}
