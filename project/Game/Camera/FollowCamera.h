#pragma once
#include "Engine.h"
#include "ICameraController.h"
#include "PropertyBinder.h"
#include "Terrain.h"

class Player;

class FollowCamera : public FS::ICameraController
{
public:
    FollowCamera(FS::Engine* engine, const FS::WorldTransform* target, FS::Terrain* terrain);
    void Initialize();
    void UpdateCamera(FS::Camera* camera) override;
    void DebugGui() override;
    void Reset(FS::Camera* camera) override;
    void PlaceBehindTarget();

    void AddRecoil(float pitchAmount, float yawAmount);

    void SetAiming(bool isAiming) { isAiming_ = isAiming; }
    bool IsAiming() const { return isAiming_; }

    // 倒れている間は操作を受け付けず、倒れた体を見下ろす位置へ寄る
    void SetDeathFocus(bool isDeathFocus);
    // false の間は、視点の入力でカメラを回せない
    void SetLookInputEnabled(bool isEnabled) { isLookInputEnabled_ = isEnabled; }

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

    // 通常カメラ設定
    float normalDistance_ = 45.0f;
    FS::Vector3 normalLookAtOffset_ = { 0.0f, 1.5f, 0.0f };
    FS::Vector3 normalShoulderOffset_ = { 0.0f, 0.0f, 0.0f };
    float normalFov_ = 0.45f;
    float normalDeadzone_ = 0.18f;

    // エイムカメラ設定
    float aimDistance_ = 18.0f;
    FS::Vector3 aimLookAtOffset_ = { 0.0f, 1.4f, 0.0f };
    FS::Vector3 aimShoulderOffset_ = { 1.8f, 0.1f, 0.0f };
    float aimFov_ = 0.35f;
    float aimTransitionSmoothTime_ = 0.12f;
    float aimDeadzone_ = 0.08f;                 
    float aimRotateSpeedMultiplier_ = 0.6f;      

    // 倒れたときのカメラ設定
    bool isDeathFocus_ = false;
    bool isLookInputEnabled_ = true;
    float deathDistance_ = 30.0f;
    FS::Vector3 deathLookAtOffset_ = { 0.0f, 0.5f, 0.0f };
    float deathPitch_ = 0.6f;
    float deathTransitionTime_ = 0.8f;
    float deathPitchVelocity_ = 0.0f;

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
    // 遊び始めとリスタートで使う、見下ろす角度
    static constexpr float kDefaultPitch = 0.3f;
    float currentPitch_ = kDefaultPitch;
    float distance_ = 45.0f;

    // 今の回転の速さ
    float yawRate_ = 0.0f;
    float pitchRate_ = 0.0f;
    float distanceVelocity_ = 0.0f;

    float rotationAccelerationTime_ = 0.1f;   
    float rotationDecelerationTime_ = 0.04f;   
    float positionLerpSpeed_ = 8.0f;

    FS::Vector3 smoothedTargetPos_;
    FS::Vector3 posVelocity_;

    FS::Quaternion currentCameraRot_;

    float minPitch_ = -0.8f;
    float maxPitch_ = 1.4f;
    float minDistance_ = 5.0f;
    float maxDistance_ = 100.0f;

    float rotateSpeedYaw_ = 2.0f;
    float rotateSpeedPitch_ = 2.0f;

    // マウスを 1 ピクセル動かしたときに回す角度
    float mouseSensitivity_ = 0.0025f;
    float mouseAimSensitivityMultiplier_ = 0.6f;
    bool isMouseYInverted_ = false;

    FS::Terrain* terrain_ = nullptr;
    float minGroundOffset_ = 0.8f;
    float groundCheckRadius_ = 0.4f;

    // リコイル用オフセット
    float recoilPitch_ = 0.0f;
    float recoilYaw_ = 0.0f;
    float recoilRecoverySpeed_ = 12.0f; 
};