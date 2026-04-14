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
        // 再生中でない、レールがない、または操作するカメラがない場合は即リターン
        if (!isPlaying_ || !currentRail_ || !camera) return;

        // 時間を進める
        currentPlayTime_ += TimeManager::GetInstance()->GetDeltaTime();

        // レールから今この瞬間にいるべきカメラの状態をもらう
        CameraKeyframe currentData = currentRail_->Evaluate(currentPlayTime_);

        //  実際のカメラに値を叩き込む
        camera->SetTranslation(currentData.position);
        camera->SetRotation(currentData.rotation);
        camera->SetFov(currentData.fov);

        // 4. 再生の終了判定（※補足参照）
        // もしレール全体の長さ（秒数）を取得する関数があれば、ここで自動停止できます。
    }

}