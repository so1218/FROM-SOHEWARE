#include "SampleSceneHori.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "ModelLoader.h"
#include "GlobalVariables.h"
#include "AnimationHandle.h"
#include "Input.h"

SampleSceneHori::SampleSceneHori(Engine* engine, Camera* camera)
{
    // ポインタを保持
    engine_ = engine;
    camera_ = camera;

    // インスタンスを作成
    player_ = std::make_unique<Player>(engine_, camera_);
    enemy_ = std::make_unique<Enemy>(engine_, camera_);
    dragonModel_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::dragon)));
    uvCheckerSprite_ = std::make_unique<Sprite>(engine_);

    animeModelData_.modelData = *ModelHandle::Get(ModelID::animatedRyu);
    animeModelData_.animation = AnimationHandle::Get(AnimationID::cube);

    // 作成したゲームオブジェクトを管理クラスに登録
    objectManager_.AddObject(std::move(player_));
    objectManager_.AddObject(std::move(enemy_));

    const char* groupName = "SampleSceneHori";
    // グループ名を追加
    GlobalVariables::GetInstance()->CreateGroup(groupName);
    GlobalVariables::GetInstance()->LoadFiles();
    GlobalVariables::GetInstance()->AddItem(groupName, "dragonModel_->GetTransform()", dragonModel_->GetTransform().translation_);
    GlobalVariables::GetInstance()->AddItem(groupName, "uvCheckerSprite_->SetPosition", uvCheckerSprite_->GetPosition());
}

void SampleSceneHori::ApplyGlobalVariables()
{
    const char* groupName = "SampleSceneHori";
    dragonModel_->GetTransform().translation_ = GlobalVariables::GetInstance()->GetVector3Value(groupName, "dragonModel_->GetTransform()");
    uvCheckerSprite_->SetPosition(GlobalVariables::GetInstance()->GetVector2Value(groupName, "uvCheckerSprite_->SetPosition"));

}

void SampleSceneHori::SaveGlobalVariables()
{
    const char* groupName = "SampleSceneHori";
    GlobalVariables::GetInstance()->SetValue(groupName, "dragonModel_->GetTransform()", dragonModel_->GetTransform().translation_);
    GlobalVariables::GetInstance()->SetValue(groupName, "uvCheckerSprite_->SetPosition", uvCheckerSprite_->GetPosition());
}


void SampleSceneHori::Initialize()
{
    // テクスチャ設定
    dragonModel_->SetTextureHandle(TextureHandle::Get(TextureID::monsterBall));

    // スプライト設定
    uvCheckerSprite_->SetPosition({ 0.0f, 0.0f });
    uvCheckerSprite_->SetSize({ 128.0f, 128.0f });
    uvCheckerSprite_->SetTextureHandle(TextureHandle::Get(TextureID::uvChecker));

    // ゲームオブジェクトの一括初期化
    objectManager_.Initialize();
}

void SampleSceneHori::Update()
{
    dragonModel_->modelData_->materialHandle.materialData->isArtWave = true;
    // ゲームオブジェクトの調整項目を一括更新
    objectManager_.ApplyGlobalVariables();
    ApplyGlobalVariables(); 
    

	engine_->UpdateAnimation(animeModelData_); 

    dragonModel_->GetTransform().scale_.x = 1.0f;
  /*  dragonModel_->GetTransform().rotation_.y += 0.01f;
    dragonModel_->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler(dragonModel_->GetTransform().rotation_);*/
    if (Input::IsKeyTriggered(DIK_L))
    {
        originalTranslation_ = dragonModel_->GetTransform().translation_;
        shake.Start(1.0f, 2.0f);
    }
    shake.Update();

    if (shake.IsJustFinished())
    {
        originalTranslation_ = dragonModel_->GetTransform().translation_;
    }

   /* dragonModel_->GetTransform().translation_.x += 0.01f;*/

    if (shake.IsActive())
    {
        dragonModel_->GetTransform().translation_ = originalTranslation_ + shake.GetOffset();
    }

    // ゲームオブジェクトの一括更新
    objectManager_.Update();

    objectManager_.SaveGlobalVariables();
    SaveGlobalVariables();

}

void SampleSceneHori::Draw()
{
    uvCheckerSprite_->Draw();
    dragonModel_->Draw();

    // ゲームオブジェクトの一括描画
    objectManager_.Draw();

    engine_->DrawModel(dragonModel_->GetTransform(), *camera_, animeModelData_, TextureHandle::Get(TextureID::uvChecker));
}

void SampleSceneHori::DebugDraw()
{
    ImGui::Begin("ホリシーン");
   
    ImGui::End();
    // ゲームオブジェクトの一括デバッグ描画
    objectManager_.DebugDraw();
}

void SampleSceneHori::Finalize()
{
}
