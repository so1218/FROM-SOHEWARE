#include "pch.h"
#include "GlobalConstants.h"
#include "BufferManager.h" 
#include "Camera.h"        
#include "TimeManager.h"
#include "Engine.h"

namespace FE
{

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

    frameData_->lightningFlashColor = { 0.3f, 0.5f, 1.0f };
    frameData_->lightningFlashIntensity = 0.0f;
}

void GlobalConstants::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix,
    const Vector3& eyePos,
    float nearClip,
    float farClip,
    const DirectionalLight& mainLight
)
{
    // 新しいVP行列を計算・上書きする前に、現在のVP行列を過去として退避
    frameData_->prevViewProj = frameData_->viewProjectionMatrix;

    // VP行列の計算
    Matrix4x4 matViewProjection = viewMatrix * projectionMatrix;
    Matrix4x4 invVP = Matrix4x4::Inverse(matViewProjection);
    Matrix4x4 invProj = Matrix4x4::Inverse(projectionMatrix);

    // シャドウマップ用のライトVP行列を転送
    frameData_->lightViewProj = mainLight.viewProj;

    // カメラ情報の転送
    frameData_->cameraWorldPosition = eyePos;
    frameData_->viewProjectionMatrix = matViewProjection;
    frameData_->invViewProj = invVP;
    frameData_->invProjMatrix = invProj;

    frameData_->viewMatrix = viewMatrix;
    frameData_->projectionMatrix = projectionMatrix;

    frameData_->cameraRight = { viewMatrix.m[0][0], viewMatrix.m[1][0], viewMatrix.m[2][0] };
    frameData_->cameraUp = { viewMatrix.m[0][1], viewMatrix.m[1][1], viewMatrix.m[2][1] };

    frameData_->nearClip = nearClip;
    frameData_->farClip = farClip;

    // メインライト情報の転送
    Vector3 dir = mainLight.direction;
    float len = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    frameData_->mainLightDirection = { dir.x / len, dir.y / len, dir.z / len };

    // 色にintensityを掛け合わせた状態
    frameData_->mainLightColor = 
    {
        mainLight.color.x * mainLight.intensity,
        mainLight.color.y * mainLight.intensity,
        mainLight.color.z * mainLight.intensity
    };

    // カメラ以外のデータも更新
    frameData_->gTime = TimeManager::GetInstance()->GetTotalTime();

    frameData_->iResolution = Vector2(1, 1);
    frameData_->screenResolution = Vector2(static_cast<float>(Engine::GetClientWidth()), static_cast<float>(Engine::GetClientHeight()));

    // フレームインデックスを進める
    // 巨大な数字になりすぎないよう、1000でループ
    frameData_->frameIndex = (frameData_->frameIndex + 1) % 1000;
}

}