#include "pch.h"
#include "SplineCameraController.h"
#include "TimeManager.h"

namespace FE
{

void SplineCameraController::Play(CameraRail* rail)
{
    // 安全対策：空のレールを渡されたら何もしない
    if (!rail) return;

    currentRail_ = rail;
    currentPlayTime_ = 0.0f; // 時間をリセット
    isPlaying_ = true;       // 再生開始
}

void SplineCameraController::UpdateCamera(Camera* camera) 
{
    if (!currentRail_ || !camera) return;

    if (isPlaying_)
    {
        float dt = TimeManager::GetInstance()->GetDeltaTime();
        currentPlayTime_ += dt;

        float total = currentRail_->GetTotalTime();
        if (currentPlayTime_ >= total)
        {
            currentPlayTime_ = total; // 最後の時間で止める
            isPlaying_ = false;       // 再生は終了
        }
    }

    CameraKeyframe frame = currentRail_->Evaluate(currentPlayTime_);
    camera->SetTranslation(frame.position);
    camera->SetRotation(frame.rotation);
    camera->SetFov(frame.fov);
}

void SplineCameraController::Draw()
{
    if (currentRail_) currentRail_->DrawDebugSpline();
}

void SplineCameraController::DebugDraw()
{
#ifdef IS_DEVELOPMENT
    if (currentRail_)
    {
        // レール側のUIを表示し、もしPlay Railが押されたら
        if (currentRail_->DebugDraw())
        {
            // Controller自身に再生開始を命令
            Play(currentRail_);
        }
    }
#endif  
}

}