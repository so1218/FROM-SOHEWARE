#pragma once

#include "Engine.h"

class Player;

class FollowCamera 
{
public:
    void Initialize(Camera* camera, Player* target);
    void Update();
    void DebugDraw();
    void SetOffset(const Vector3& offset) { offset_ = offset; }
    float yaw_ = 0.0f;          // カメラの回転角（Y軸）
private:
    Camera* camera_ = nullptr;
    Player* target_ = nullptr;
    Vector3 offset_ = { 0.0f, 3.0f, -90.0f }; // カメラ位置の相対オフセット

    
    float distance_ = 50.0f;    // プレイヤーとの距離
    float height_ = 5.0f;       // プレイヤーからの高さ
   
    // 補間用
    Vector3 currentCameraPos_;    // 現在のカメラ位置
    Quaternion currentCameraRot_; // 現在のカメラ回転
    float interpSpeed_; // 補間速度 
};
