#include "SampleSceneMikami.h"
#include "TextureHandle.h"
#include "ModelHandle.h"
#include "ImGuiManager.h"
#include "ModelLoader.h"
#include "GlobalVariables.h"

SampleSceneMikami::SampleSceneMikami(Engine* engine, Camera* camera)
{
    // ポインタを保持
    engine_ = engine;
    camera_ = camera;

    // インスタンスを作成
    player_ = std::make_unique<Player>();
    enemy_ = std::make_unique<Enemy>();
    dragonModel_ = std::make_unique<Model>(engine_, camera_, std::move(ModelHandle::Get(ModelID::dragon)));
    uvCheckerSprite_ = std::make_unique<Sprite>(engine_);

    // 作成したゲームオブジェクトを管理クラスに登録
    objectManager_.AddObject(std::move(player_));
    objectManager_.AddObject(std::move(enemy_));

    const char* groupName = "SampleSceneMikami";
    // グループ名を追加
    GlobalVariables::GetInstance()->CreateGroup(groupName);
    GlobalVariables::GetInstance()->LoadFiles();
    GlobalVariables::GetInstance()->AddItem(groupName, "dragonModel_->GetTransform()", dragonModel_->GetTransform().translation_);

}

void SampleSceneMikami::ApplyGlobalVariables()
{
    const char* groupName = "SampleSceneMikami";
    dragonModel_->GetTransform().translation_ = GlobalVariables::GetInstance()->GetVector3Value(groupName, "dragonModel_->GetTransform()");
}


void SampleSceneMikami::Initialize()
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

void SampleSceneMikami::Update()
{
    dragonModel_->GetTransform().scale_.x = 1.0f;
    dragonModel_->GetTransform().rotation_.y += 0.01f;
    dragonModel_->GetTransform().rotationQuaternion_ = Quaternion::QuaternionFromEuler(dragonModel_->GetTransform().rotation_);

    // ゲームオブジェクトの調整項目を一括更新
    objectManager_.ApplyGlobalVariables();

    // ゲームオブジェクトの一括更新
    objectManager_.Update();

    ApplyGlobalVariables();
}

void SampleSceneMikami::Draw()
{
    uvCheckerSprite_->Draw();
    dragonModel_->Draw();

    // ゲームオブジェクトの一括描画
    objectManager_.Draw();
}

void SampleSceneMikami::DebugDraw()
{
    // ゲームオブジェクトの一括デバッグ描画
    objectManager_.DebugDraw();
}

void SampleSceneMikami::Finalize()
{
}
