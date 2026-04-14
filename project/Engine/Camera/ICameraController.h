#pragma once
#include "Camera.h"

namespace FE
{

// カメラの基底クラス
class ICameraController
{
public:
    virtual ~ICameraController() = default;

    // 実際のカメラに自分の計算結果（位置・回転）を書き込む
    virtual void UpdateCamera(Camera* camera) = 0;
};

}