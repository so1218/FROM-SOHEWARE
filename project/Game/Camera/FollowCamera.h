#pragma once
#include "Engine.h"
#include "ICameraController.h"
#include "PropertyBinder.h"
#include "Terrain.h"

class Player;

class FollowCamera : public FS::ICameraController
{
public:
    FollowCamera(FS::Engine* engine, const FS::WorldTransform* target);
    void Initialize();
    void UpdateCamera(FS::Camera* camera) override;
    void DebugGui() override;
    void Reset(FS::Camera* camera) override;
    // 追いかける相手の真後ろを向き先にする
    void PlaceBehindTarget();

    void AddRecoil(float pitchAmount, float yawAmount);

    void SetTerrain(FS::Terrain* terrain) { terrain_ = terrain; }
    void SetAiming(bool isAiming) { isAiming_ = isAiming; }
    bool IsAiming() const { return isAiming_; }

private:
    float SmoothDamp(float current, float target, float& currentVelocity,
        float smoothTime, float deltaTime, float maxSpeed = 1000.0f);
    float ApproachRotationRate(float currentRate, float targetRate, float maxRate, float deltaTime) const;

private:
    FS::Engine* engine_;
    const FS::WorldTransform* target_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    bool isAiming_ = false;

    // スティック最大値
    static constexpr float kMaxStickValue = 32767.0f;

    // 通常カメラ設定
    float normalDistance_ = 45.0f;
    FS::Vector3 normalLookAtOffset_ = { 0.0f, 1.5f, 0.0f };
    FS::Vector3 normalShoulderOffset_ = { 0.0f, 0.0f, 0.0f };
    float normalFov_ = 0.45f;
    float normalDeadzone_ = 6000.0f;

    // エイムカメラ設定
    float aimDistance_ = 18.0f;
    FS::Vector3 aimLookAtOffset_ = { 0.0f, 1.4f, 0.0f };
    FS::Vector3 aimShoulderOffset_ = { 1.8f, 0.1f, 0.0f };
    float aimFov_ = 0.35f;
    float aimTransitionSmoothTime_ = 0.12f;
    float aimDeadzone_ = 2500.0f;                 
    float aimRotateSpeedMultiplier_ = 0.6f;       // エイム時感度倍率

    FS::Vector3 currentShoulderOffset_ = { 0.0f, 0.0f, 0.0f };
    FS::Vector3 shoulderOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };

    FS::Vector3 currentLookAtOffset_ = { 0.0f, 1.5f, 0.0f };
    FS::Vector3 lookAtOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };

    float currentFov_ = 0.45f;
    float fovVelocity_ = 0.0f;

    // エイム中は被写界深度を消す。1 で通常、0 で無し
    float dofStrength_ = 1.0f;
    float dofStrengthVelocity_ = 0.0f;

    // 回転・距離・補間変数
    float targetDistance_ = 45.0f;

    float currentYaw_ = 0.0f;
    float currentPitch_ = 0.3f;
    float distance_ = 45.0f;

    // 今の回転の速さ (ラジアン / 秒)。スティックを離すとすぐ 0 に戻る
    float yawRate_ = 0.0f;
    float pitchRate_ = 0.0f;
    float distanceVelocity_ = 0.0f;

    float rotationAccelerationTime_ = 0.1f;    // 回し始めてから最高速度になるまで (秒)
    float rotationDecelerationTime_ = 0.04f;   // 最高速度から止まるまで (秒)
    float zoomSmoothTime_ = 0.2f;
    float positionLerpSpeed_ = 8.0f;

    FS::Vector3 smoothedTargetPos_;
    FS::Vector3 posVelocity_;

    FS::Quaternion currentCameraRot_;
    FS::Vector3 lookAtOffset_ = { 0.0f, 1.5f, 0.0f };

    float minPitch_ = -0.8f;
    float maxPitch_ = 1.4f;
    float minDistance_ = 5.0f;
    float maxDistance_ = 100.0f;

    float rotateSpeedYaw_ = 2.0f;
    float rotateSpeedPitch_ = 2.0f;

    FS::Terrain* terrain_ = nullptr;
    float minGroundOffset_ = 0.8f;
    float groundCheckRadius_ = 0.4f;

    // リコイル用オフセット
    float recoilPitch_ = 0.0f;
    float recoilYaw_ = 0.0f;
    float recoilRecoverySpeed_ = 12.0f; 
};