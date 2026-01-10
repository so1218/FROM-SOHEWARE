#pragma once
#include "Structures.h"
#include "WorldTransform.h"
#include "MathUtils.h"

class Camera
{
public:
    Camera() { Initialize(); }

    void Initialize();

    // WorldTransform関連
    WorldTransform& GetWorldTransform() { return worldTransform_; }
    const WorldTransform& GetWorldTransform() const { return worldTransform_; }

    void SetWorldTransform(const WorldTransform& wt) {
        worldTransform_ = wt;
        UpdateViewProjectionMatrix();
    }

    Vector3 GetTranslation() const { return worldTransform_.translation_; }

    void SetTranslation(const Vector3& translation) {
        worldTransform_.translation_ = translation;
        UpdateViewProjectionMatrix();
    }

    Quaternion GetRotation() const { return worldTransform_.rotationQuaternion_; }

    void SetRotation(const Quaternion& rotation) {
        worldTransform_.SetRotation(rotation);
        UpdateViewProjectionMatrix();
    }

    Vector3 GetWorldRotationEuler() const { return worldTransform_.rotation_; }
    void SetWorldRotationEuler(const Vector3& euler) {
        worldTransform_.rotation_ = euler;
        Quaternion q = Quaternion::QuaternionFromEuler(euler);
        worldTransform_.SetRotation(q);
        worldTransform_.UpdateMatrix();
    }

    // カメラパラメータ
    float GetFov() const { return fovY_; }
    void SetFov(float fovY) {
        fovY_ = fovY;
        UpdateProjectionMatrix();
        UpdateViewProjectionMatrix();
    }

    float GetAspectRatio() const { return aspectRatio_; }
    void SetAspectRatio(float aspectRatio) {
        aspectRatio_ = aspectRatio;
        UpdateProjectionMatrix();
        UpdateViewProjectionMatrix();
    }

    float GetNearClip() const { return nearClip_; }
    void SetNearClip(float nearClip) {
        nearClip_ = nearClip;
        UpdateProjectionMatrix();
        UpdateViewProjectionMatrix();
    }

    float GetFarClip() const { return farClip_; }
    void SetFarClip(float farClip) {
        farClip_ = farClip;
        UpdateProjectionMatrix();
        UpdateViewProjectionMatrix();
    }

    // カメラの前方ベクトルを取得
    Vector3 GetForward() const 
    {
        // ワールド行列のZ軸成分が前方ベクトル
        Vector3 forward;
        forward.x = worldTransform_.matWorld_.m[2][0];
        forward.y = worldTransform_.matWorld_.m[2][1];
        forward.z = worldTransform_.matWorld_.m[2][2];
        return forward;
    }

    // 行列取得
    const Matrix4x4& GetViewMatrix() const { return matView_; }
    const Matrix4x4& GetProjectionMatrix() const { return matProjection_; }
    const Matrix4x4& GetViewProjectionMatrix() const { return matViewProjection_; }

    void SetViewMatrix(const Matrix4x4& view)
    {
        matView_ = view;
        matViewProjection_ = matView_ * matProjection_;
    }

    void SetProjectionMatrix(const Matrix4x4& projection)
    {
        matProjection_ = projection;
        matViewProjection_ = matView_ * matProjection_;
    }

    void SetViewProjectionMatrix(const Matrix4x4& vp) {
        matViewProjection_ = vp;
    }

    // 更新関数
    void UpdateViewMatrix() {
        worldTransform_.UpdateMatrix();
        matView_ = Matrix4x4::Inverse(worldTransform_.matWorld_);
    }

    void UpdateProjectionMatrix() {
        matProjection_ = Matrix4x4::MakePerspectiveFov(fovY_, aspectRatio_, nearClip_, farClip_);
    }

    void UpdateViewProjectionMatrix() {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
        matViewProjection_ = matView_ * matProjection_;
    }

private:
    WorldTransform worldTransform_;

    float fovY_ = 0.45f;
    float aspectRatio_ = 16.0f / 9.0f;
    float nearClip_ = 0.1f;
    float farClip_ = 1000.0f;

    Matrix4x4 matView_;
    Matrix4x4 matProjection_;
    Matrix4x4 matViewProjection_;
};
