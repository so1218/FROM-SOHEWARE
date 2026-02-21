#include "GlobalConstants.h"
#include "BufferManager.h" 
#include "Camera.h"        
#include "TimeManager.h"
#include "Engine.h"
#include <cassert>         

void GlobalConstants::Initialize(ID3D12Device* device)
{
    // 定数バッファを作成
    constantBuffer_ = BufferManager::CreateBufferResource(device, sizeof(FrameData));

    // マッピング
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&frameData_));

    // 初期化
    memset(frameData_, 0, sizeof(FrameData));

    // デフォルト値
    frameData_->nearClip = 0.1f;
    frameData_->farClip = 1000.0f;
    frameData_->viewProjectionMatrix = Matrix4x4::MakeIdentity();
}

void GlobalConstants::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix,
    const Vector3& eyePos,
    float nearClip,
    float farClip
)
{
    // VP行列の計算
    Matrix4x4 matViewProjection = viewMatrix * projectionMatrix;

    Matrix4x4 invVP = Matrix4x4::Inverse(matViewProjection);

    // カメラ情報の転送
    frameData_->cameraWorldPosition = eyePos;
    frameData_->viewProjectionMatrix = matViewProjection;
    frameData_->invViewProj = invVP;

    frameData_->cameraRight = { viewMatrix.m[0][0], viewMatrix.m[1][0], viewMatrix.m[2][0] };
    frameData_->cameraUp = { viewMatrix.m[0][1], viewMatrix.m[1][1], viewMatrix.m[2][1] };

    frameData_->nearClip = nearClip;
    frameData_->farClip = farClip;

    // カメラ以外のデータも更新
    frameData_->gTime = TimeManager::GetInstance()->GetTotalTime();

    frameData_->iResolution = Vector2(1, 1);
    frameData_->screenResolution = Vector2(static_cast<float>(kClientWidth), static_cast<float>(kClientHeight));
}