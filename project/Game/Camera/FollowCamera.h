#pragma once
#include "Engine.h"
#include "ICameraController.h"
#include "PropertyBinder.h"
#include "Terrain.h"

class Player;

class FollowCamera : public FE::ICameraController
{
public:
    FollowCamera(FE::Engine* engine, const FE::WorldTransform* target);
    void Initialize();
    void UpdateCamera(FE::Camera* camera) override;
    void DebugDraw() override; 
    void Reset(FE::Camera* camera) override;

    // 地形情報をセットする関数
    void SetTerrain(FE::Terrain* terrain) { terrain_ = terrain; }

    // エイム状態の切り替え
    void SetAiming(bool isAiming) { isAiming_ = isAiming; }
    bool IsAiming() const { return isAiming_; }

private:
    // スムーズ補間関数
    float SmoothDamp(float current, float target, float& currentVelocity,
        float smoothTime, float deltaTime, float maxSpeed = 1000.0f);
    float SmoothDampAngle(float current, float target, float& currentVelocity,
        float smoothTime, float deltaTime, float maxSpeed = 1000.0f);
   
private:
    FE::Engine* engine_;
    const FE::WorldTransform* target_ = nullptr;
    std::unique_ptr<FE::PropertyBinder> binder_;

    bool isAiming_ = false; // エイム状態フラグ

    // 通常カメラ設定
    float normalDistance_ = 45.0f;
    FE::Vector3 normalLookAtOffset_ = { 0.0f, 1.5f, 0.0f };
    FE::Vector3 normalShoulderOffset_ = { 0.0f, 0.0f, 0.0f }; // 中央配置
    float normalFov_ = 0.45f;

    // エイムカメラ設定
    float aimDistance_ = 18.0f;                                // プレイヤーへ接近
    FE::Vector3 aimLookAtOffset_ = { 0.0f, 1.4f, 0.0f };      // 注視点高さ
    FE::Vector3 aimShoulderOffset_ = { 1.8f, 0.1f, 0.0f };     // 右肩越しオフセット
    float aimFov_ = 0.35f;                                     // FOVを絞ってズーム

    // エイム遷移の補間パラメータ
    float aimTransitionSmoothTime_ = 0.12f; // エイム切替にかかる補間時間

    FE::Vector3 currentShoulderOffset_ = { 0.0f, 0.0f, 0.0f };
    FE::Vector3 shoulderOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };

    FE::Vector3 currentLookAtOffset_ = { 0.0f, 1.5f, 0.0f };
    FE::Vector3 lookAtOffsetVelocity_ = { 0.0f, 0.0f, 0.0f };

    float currentFov_ = 0.45f;
    float fovVelocity_ = 0.0f;

    // 回転・距離・補間変数
    float targetYaw_ = 0.0f;
    float targetPitch_ = 0.3f;
    float targetDistance_ = 45.0f;

    float currentYaw_ = 0.0f;
    float currentPitch_ = 0.3f;
    float distance_ = 45.0f;

    float yawVelocity_ = 0.0f;
    float pitchVelocity_ = 0.0f;
    float distanceVelocity_ = 0.0f;

    float rotationSmoothTime_ = 0.1f;
    float zoomSmoothTime_ = 0.2f;
    float positionLerpSpeed_ = 8.0f;

    FE::Vector3 smoothedTargetPos_;
    FE::Vector3 posVelocity_;

    FE::Quaternion currentCameraRot_;
    FE::Vector3 lookAtOffset_ = { 0.0f, 1.5f, 0.0f };

    float minPitch_ = -0.8f;
    float maxPitch_ = 1.4f;
    float minDistance_ = 5.0f;
    float maxDistance_ = 100.0f;

    float rotateSpeedYaw_ = 2.0f;
    float rotateSpeedPitch_ = 2.0f;

    FE::Terrain* terrain_ = nullptr;
    float minGroundOffset_ = 0.8f;
    float groundCheckRadius_ = 0.4f;
};