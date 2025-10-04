#include "CameraController.h"
#include "Player.h"

#include <algorithm>

void CameraController::Initialize(Camera* camera)
{
	camera_ = camera;
}

void CameraController::Update()
{
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットと追従対象の速度からカメラの目標座標を計算
	//targetPosition.x = targetWorldTransform.translation_.x + targetoffset_.x + target_->GetVelocity().x * kVelocityBias;
	//targetPosition.y = targetWorldTransform.translation_.y + targetoffset_.y + target_->GetVelocity().y * kVelocityBias;
	//targetPosition.z = targetWorldTransform.translation_.z + targetoffset_.z + target_->GetVelocity().z * kVelocityBias;

	// 座標保管によりゆったり追従
	camera_->SetTranslation(Vector3::Lerp(camera_->GetTranslation(), targetPosition, kInterpolationRate));

	// 追従対象が画面外に出ないように補正
	//camera_->translation_.x = std::max(camera_->translation_.x, targetPosition.x + margin.left);
	//camera_->translation_.x = std::min(camera_->translation_.x, targetPosition.x + margin.right);
	//camera_->translation_.y = std::max(camera_->translation_.y, targetPosition.y + margin.bottom);
	//camera_->translation_.y = std::min(camera_->translation_.y, targetPosition.y + margin.top);

	Vector3 clampedTranslation = camera_->GetTranslation();
	clampedTranslation.x = std::clamp(clampedTranslation.x, movableArea_.left, movableArea_.right);
	clampedTranslation.y = std::clamp(clampedTranslation.y, movableArea_.bottom, movableArea_.top);
	camera_->SetTranslation(clampedTranslation);

	camera_->GetWorldTransform().UpdateMatrix();
	// 行列を更新する
	camera_->UpdateViewProjectionMatrix();
}

void CameraController::Reset()
{
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	Vector3 offsetPos = targetWorldTransform.translation_ + targetoffset_;
	camera_->SetTranslation(offsetPos);
}