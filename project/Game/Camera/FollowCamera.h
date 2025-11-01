#pragma once

#include "Engine.h"
#include "ShakeEffect.h"

class Player;

class FollowCamera 
{
public:
    void Initialize(Camera* camera, Player* target);
    void Update();
    void DebugDraw();

    void StartShake(float duration, float intensity);

    float yaw_ = 0.0f;          // カメラの回転角（Y軸）
    float pitch_ = 0.3f;

private:
    Camera* camera_ = nullptr;
    Player* target_ = nullptr;
    
    float distance_ = 50.0f;    // プレイヤーとの距離
    float targetDistance_ = 50.0f; // 目標の距離
    float zoomLerpSpeed_ = 10.0f; // ズームの補間速度
   
    // 補間用
    Vector3 currentCameraPos_;    // 現在のカメラ位置
    Quaternion currentCameraRot_; // 現在のカメラ回転
    float interpSpeed_; // 補間速度 

    Vector3 lookAtOffset_ = { 0.0f, 1.5f, 0.0f };

    ShakeEffect shakeEffect_;
};
