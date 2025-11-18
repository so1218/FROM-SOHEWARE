#include "TitleScene.h"
#include "SceneManager.h"
#include "PlayScene.h"
#include "TextureHandle.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "ModelHandle.h"
#include "TimeManager.h"
#include "ModelLoader.h"
#include "Collision.h"

TitleScene::TitleScene(Engine* engine, Camera* camera)
{
    // ポインタを保存
    engine_ = engine;
    camera_ = camera;
    sprite_ = std::make_unique<Sprite>(engine_);
}

void TitleScene::Initialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0, 0, -6.6f));
    sprite_->SetPosition({ 640, 360 });
    sprite_->SetSize({ 640, 360 });
    sprite_->SetTextureHandle(TextureHandle::Get(TextureID::uvChecker));
}

void TitleScene::Update()
{
	// シーン切り替えの入力検出
	if (Input::GetInstance().IsKeyTriggered(DIK_SPACE))
	{
		// シーンマネージャーを通じてシーン切り替えをリクエスト
		sceneManager_->RequestSceneChange(SceneID::Sample);
	}
}

void TitleScene::Draw()
{
    sprite_->Draw();
}

void TitleScene::DebugDraw()
{
    ImGui::Begin("タイトルシーン");

    ImGui::End();
}

void TitleScene::Finalize()
{
   
}