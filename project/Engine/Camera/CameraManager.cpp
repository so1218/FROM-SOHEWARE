#include "pch.h"
#include "CameraManager.h"

namespace FE
{

CameraManager::CameraManager(Camera* camera) : mainCamera_(camera)
{
    // マネージャー生成時に、演出専用のコントローラーを作っておく
    splineController_ = std::make_unique<SplineCameraController>();
}

void CameraManager::ChangeController(ICameraController* controller)
{

    if (controller != splineController_.get()) {
        defaultController_ = controller;
    }
    currentController_ = controller;
}

void CameraManager::AddRail(const std::string& name, std::unique_ptr<CameraRail> rail)
{
    // レールを登録
    rails_[name] = std::move(rail);
}

void CameraManager::PlayRail(const std::string& name)
{
    if (rails_.count(name))
    {
        // 現在のカメラを演出用に切り替え
        ChangeController(splineController_.get());
        splineController_->Play(rails_[name].get());
    }
}

void CameraManager::Update()
{
    if (!currentController_ || !mainCamera_) return;

    // 1. 各コントローラーに基本位置を決めさせる（FollowCamera や SplineCamera）
    currentController_->UpdateCamera(mainCamera_);

    // 2. シェイクを更新
    shake_.Update();

    // 3. 最終的な座標にシェイクのオフセットを「上乗せ」する
    if (shake_.IsActive())
    {
        Vector3 currentPos = mainCamera_->GetTranslation();
        mainCamera_->SetTranslation(currentPos + shake_.GetOffset());
    }

    // 演出カメラの自動切り戻し処理（既存のコード）
    if (currentController_ == splineController_.get())
    {
        if (!splineController_->IsPlaying())
        {
            ChangeController(defaultController_);
        }
    }
}

void CameraManager::RequestShake(float duration, float intensity)
{
    shake_.Start(duration, intensity);
}

void CameraManager::DebugDraw()
{
    if (currentController_) {
        currentController_->DebugDraw();
    }

    // レールのUIと再生管理の自動化
    for (auto& pair : rails_)
    {
        // 登録されている全レールのUIを表示
        // もしPlay Railボタンが押されたら
        if (pair.second->DebugDraw())
        {
            // 演出カメラに切り替えて、再生スタート
            currentController_ = splineController_.get(); // 記憶を上書きせず切り替え
            splineController_->Play(pair.second.get());
        }
    }
}

void CameraManager::Draw()
{
    if (currentController_) 
    {
        currentController_->Draw();
    }
}

CameraRail* CameraManager::GetRail(const std::string& name)
{
    if (rails_.count(name)) return rails_[name].get();
    return nullptr;
}

}