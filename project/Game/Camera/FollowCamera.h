#pragma once

#include "Engine.h"
#include "ShakeEffect.h"
#include "GameObject.h"
#include "PropertyBinder.h"

class Player;

class FollowCamera : public GameObject
{
public:
    FollowCamera(Engine* engine, Camera* camera, Player* target);
    void Initialize() override;
    void Update() override;
    void DebugDraw() override;
    void Draw() override {}
   
    void StartShake(float duration, float intensity);

private:
    Player* target_ = nullptr;
    Camera* camera_ = nullptr;

    std::unique_ptr<PropertyBinder> binder_;

    // 目標値
    float targetYaw_ = 0.0f;
    float targetPitch_ = 0.3f;
    float targetDistance_ = 50.0f;

    // 現在値
    float currentYaw_ = 0.0f;
    float currentPitch_ = 0.3f;
    float distance_ = 50.0f; // 現在のズーム距離

    // 補間用速度
    float yawVelocity_ = 0.0f;
    float pitchVelocity_ = 0.0f;
    float distanceVelocity_ = 0.0f;

    // スムージング設定（秒）
    float rotationSmoothTime_ = 0.1f; // 回転が追従する時間
    float zoomSmoothTime_ = 0.2f;     // ズームが追従する時間
    float positionLerpSpeed_ = 8.0f;  // ターゲット位置の追従速度

    // ターゲット位置の補間
    Vector3 smoothedTargetPos_;   // 補間されたプレイヤー中心位置
    Vector3 posVelocity_;         // 位置補間用速度

    // カメラ制御
    Quaternion currentCameraRot_; // 現在のカメラ回転
    Vector3 lookAtOffset_ = { 0.0f, 1.5f, 0.0f }; // プレイヤーを見上げるオフセット
    ShakeEffect shakeEffect_;     // カメラシェイク効果

    // 制限値
    float minPitch_ = -0.8f;
    float maxPitch_ = 1.4f;
    float minDistance_ = 5.0f;
    float maxDistance_ = 100.0f;

    float rotateSpeedYaw_ = 2.0f;   // 左右回転速度
    float rotateSpeedPitch_ = 2.0f; // 上下回転速度

    // スムーズ補間関数
    float SmoothDamp(float current, float target, float& currentVelocity,
        float smoothTime, float deltaTime, float maxSpeed = 1000.0f);
    float SmoothDampAngle(float current, float target, float& currentVelocity,
        float smoothTime, float deltaTime, float maxSpeed = 1000.0f);
};