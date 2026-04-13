#include "pch.h"
#include "TrackCamera.h"
#include "TimeManager.h"

namespace FE
{

void TrackCamera::Update() 
{
    //currentT_ += moveSpeed_ * TimeManager::GetInstance()->GetUnscaledDeltaTime();
    //if (currentT_ > 1.0f) currentT_ = 1.0f; // ループしない場合

    //// 位置の更新
    //Vector3 pos = path_.GetPositionAt(currentT_);
    //camera_->SetTranslation(pos);

    //// 回転の更新
    //Vector3 forwardPos = path_.GetPositionAt(currentT_ + 0.01f);
    //Vector3 dir = (forwardPos - pos).Normalize();
    //camera_->SetRotation(Quaternion::LookRotation(dir, { 0, 1, 0 }));
}

void TrackCamera::DebugDraw() 
{
    // ImGuiで制御点の追加・編集UI
    // デバッグラインで points を繋ぐ曲線を3D空間に描画
}

}