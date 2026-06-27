#include "pch.h"
#include "TestSceneHori.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"
#include "Grid.h"
#include "Ground.h"
#include "GrassField.h"
#include "SceneManager.h"
#include "AudioPlayer.h"
#include "Orb.h"
#include "IEditorCommand.h"
#include "PlaceModelCommand.h"
#include "MoveModelCommand.h"

using namespace FE;

TestSceneHori::TestSceneHori(Engine* engine)
    : BaseScene(engine)
{
	// ゲームオブジェクトの生成・登録
    player_ = objectManager_.Create<Player>(engine_, camera_.get());
    followCamera_ = std::make_unique<FollowCamera>(engine_, &player_->GetTransform());
    //objectManager_.Create<Grid>(engine_);
    objectManager_.Create<Ground>(engine_);
    bubble_ = objectManager_.Create<Bubble>(engine_);
    objectManager_.Create<GrassField>(engine_, player_);

    //player_->SetFollowCamera(followCamera_);
}

void TestSceneHori::OnInitialize()
{
    // ライトの設定
    engine_->GetLightManager()->GetDirectionalLightData()[0].enable = true;
    engine_->GetLightManager()->GetDirectionalLightData()[0].direction = { 2.6f,-0.4f,1.4f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].color = { 1.0f,193.0f / 255.0f,96.0f / 255.0f,1.0f };
    engine_->GetLightManager()->GetDirectionalLightData()[0].intensity = 0.4f;
    engine_->GetPostEffectManager()->GetCombineSettings()->enableVolumetricFog = true;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->noiseIntensity = 0.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->scatteringIntensity = 5.0f;
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->ambientLight = { 9.0f / 255.0f,9.0f / 255.0f,9.0f / 255.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogSettings()->maxDistance = 500.0f;

    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().clear();
    
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().push_back(VolumetricFogPass::FogVolumeData());
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData().push_back(VolumetricFogPass::FogVolumeData());
    
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].type = 1;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].position = { 50.0f,20.0f,-50.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].scale = { 50.0f,20.0f,50.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].color = { 65.0f / 255.0f,164.0f / 255.0f,252.0f / 255.0f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].density = 0.5f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].blendDistance = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].windDirection = { 1.0f,-0.2f,0.7f };
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].windSpeed = 0.3f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[0].coverage = 0.95f;
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
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].coverage = 0.95f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].worleyWeight = 0.95f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].noiseIntensity = 0.9f;
    engine_->GetPostEffectManager()->GetVolumetricFogPass()->GetFogVolumesData()[1].noiseScale = { 0.06f,0.06f,0.06f };
    engine_->GetPostEffectManager()->GetBrightSettings()->threshold = 0.4f;
    engine_->GetPostEffectManager()->GetBrightSettings()->intensity = 1.1f;
    engine_->GetPostEffectManager()->GetCombineSettings()->enableDoF = true;
    engine_->GetPostEffectManager()->GetDoFSettings()->focusDistance = 100.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->focusRange = 33.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->bokehHighlightIntensity = 3.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->transitionRange = 45.0f;
    engine_->GetPostEffectManager()->GetDoFSettings()->bokehRadius = 1.5f;
    //testSceneEmitter_ = engine_->GetParticleSystem()->CreateEmitter("testScene");
    //engine_->GetParticleSystem()->AddEmitter(std::move(testSceneEmitter_));
    //auraEmitter_ = engine_->GetParticleSystem()->CreateEmitter("aura");
    //engine_->GetParticleSystem()->AddEmitter(std::move(auraEmitter_));

    // 生成・初期化
    auto openingRail = std::make_unique<CameraRail>(engine_, camera_.get(), "HoriScene_Opening");
    openingRail->Initialize();

    for (int i = 0; i < 3; ++i)
    {
        objectManager_.Create<Orb>(engine_, i);
    }

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

    EditorHistoryManager::GetInstance().Update();

#ifdef IS_DEVELOPMENT
    // ドラッグ＆ドロップされたモデルの受け取りと生成
    std::string droppedName = engine_->GetDebugGuiManager()->ConsumeDroppedModelName();

    if (!droppedName.empty())
    {
        auto newModel = std::make_unique<FE::Model>(engine_, droppedName);
        WorldTransform transform;
        transform.translation_ = { 0.0f, 0.0f, 0.0f };
        newModel->SetTransform(transform);

        // 直接 vector に push せず、コマンドを介して安全に生成・登録
        auto command = std::make_unique<PlaceModelCommand>(this, std::move(newModel));
        EditorHistoryManager::GetInstance().AddAndExecute(std::move(command));

        OutputDebugStringA(("[Scene] Placed model via Command: " + droppedName + "\n").c_str());
    }
#endif
}

void TestSceneHori::OnDraw()
{
    for (const auto& model : placedModels_)
    {
        model->Draw();

        // ギズモ処理前の座標を退避
        Vector3 posBeforeGizmo = model->GetTransform().translation_;

        // ギズモを描画
        ImGuiManager::DrawGizmo(model->GetTransform());

        // ギズモ処理後の座標
        Vector3 posAfterGizmo = model->GetTransform().translation_;

        // IsUsing を使わず、実際に座標が変わったかでドラッグ開始を判定
        bool isMovedThisFrame = (posBeforeGizmo.x != posAfterGizmo.x ||
            posBeforeGizmo.y != posAfterGizmo.y ||
            posBeforeGizmo.z != posAfterGizmo.z);

        if (isMovedThisFrame)
        {
            if (!isGizmoUsingLastFrame_)
            {
                // ドラッグ開始
                gizmoOldTranslation_ = posBeforeGizmo;
                gizmoTargetModel_ = model.get();
                isGizmoUsingLastFrame_ = true;

                OutputDebugStringA("[Gizmo] Drag Started by Diff!\n");
            }
        }

        // ドラッグ中なら、左クリックを離した瞬間をドラッグ終了
        if (isGizmoUsingLastFrame_ && gizmoTargetModel_ == model.get())
        {
            // ImGui の機能で左クリック(0)が離された瞬間を検知
            if (ImGui::IsMouseReleased(0))
            {
                Vector3 newTranslation = posAfterGizmo;

                std::string log = "[Gizmo] End Drag. OldX: " + std::to_string(gizmoOldTranslation_.x) +
                    ", NewX: " + std::to_string(newTranslation.x) + "\n";
                OutputDebugStringA(log.c_str());

                // 実際に移動していればコマンド化
                auto moveCmd = std::make_unique<MoveModelCommand>(model.get(), gizmoOldTranslation_, newTranslation);

                // コマンド登録前に元の位置に戻す
                model->GetTransform().translation_ = gizmoOldTranslation_;

                EditorHistoryManager::GetInstance().AddAndExecute(std::move(moveCmd));

                isGizmoUsingLastFrame_ = false;
                gizmoTargetModel_ = nullptr;
            }
        }
    }
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

std::unique_ptr<FE::Model> TestSceneHori::RemovePlacedModel(FE::Model* targetPtr)
{
    auto it = std::find_if(placedModels_.begin(), placedModels_.end(),
        [&](const auto& m) { return m.get() == targetPtr; });

    if (it != placedModels_.end())
    {
        std::unique_ptr<FE::Model> ret = std::move(*it);
        placedModels_.erase(it);
        return ret; // 所有権を呼び出し元に返す
    }
    return nullptr;
}