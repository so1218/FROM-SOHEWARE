#include "pch.h"
#include "CameraManager.h"

namespace FE
{

void CameraManager::ChangeController(ICameraController* newController)
{
    activeController_ = newController;

    // 切り替えた瞬間にResetを呼んで、カメラ位置が飛ぶのを防ぐ
    if (activeController_ && mainCamera_)
    {
        activeController_->Reset(mainCamera_);
    }
}

void CameraManager::Update()
{
    if (activeController_ && mainCamera_)
    {
        // 現在アクティブなコントローラーに、実際のカメラを操作させる
        activeController_->UpdateCamera(mainCamera_);
    }
}

void CameraManager::RequestShake(float duration, float intensity)
{
    if (activeController_)
    {
        activeController_->StartShake(duration, intensity);
    }
}

void CameraManager::DebugDraw()
{
    if (activeController_)
    {
        activeController_->DebugDraw(); // 今アクティブなコントローラーのUIを表示
    }
}

void CameraManager::Draw()
{
    if (activeController_) activeController_->Draw();
}

}