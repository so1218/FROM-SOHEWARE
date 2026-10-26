#include "pch.h"
#include "CameraManager.h"

namespace FS
{

CameraManager::CameraManager(Camera* camera) : mainCamera_(camera)
{
}

void CameraManager::ChangeController(ICameraController* controller)
{
    defaultController_ = controller;
    currentController_ = controller;
}

void CameraManager::Update()
{
    if (!currentController_ || !mainCamera_) return;

    currentController_->UpdateCamera(mainCamera_);

    // シェイクを更新
    shake_.Update();

    // 最終的な座標にシェイクのオフセットを上乗せ
    if (shake_.IsActive())
    {
        Vector3 currentPos = mainCamera_->GetTranslation();
        mainCamera_->SetTranslation(currentPos + shake_.GetOffset());
    }
}

void CameraManager::RequestShake(float duration, float intensity)
{
    shake_.Start(duration, intensity);
}

void CameraManager::DebugDraw()
{
    if (currentController_)
    {
        currentController_->DebugDraw();
    }
}

void CameraManager::Draw()
{
    if (currentController_) 
    {
        currentController_->Draw();
    }
}

}