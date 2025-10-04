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
}

void TitleScene::Initialize()
{
    camera_->Initialize();
    camera_->SetTranslation(Vector3(0, 0, -6.6f));
}

void TitleScene::Update()
{
  
}

void TitleScene::Draw()
{
    
}

void TitleScene::DebugDraw()
{
    ImGui::Begin("タイトルシーン");

    ImGui::End();
}

void TitleScene::Finalize()
{
   
}