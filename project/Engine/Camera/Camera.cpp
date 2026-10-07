#include "pch.h"
#include "Camera.h"
#include "PropertyBinder.h"

namespace FE
{

void Camera::Initialize()
{
    // 初期化処理
    worldTransform_.Initialize();

    // カメラの初期位置を設定
    worldTransform_.translation_ = { 0.0f, 5.0f, -50.0f };

    // カメラパラメータの初期化
    fovY_ = 0.45f;
    aspectRatio_ = 16.0f / 9.0f;
    nearClip_ = 0.1f;
    farClip_ = 500.0f;

    // 行列を初期化しておく
    UpdateProjectionMatrix();
    UpdateViewProjectionMatrix();
}

void Camera::BindProperties(PropertyBinder& binder, const std::string& prefix)
{
    prefix_ = prefix; // プレフィックスを保存しておく
    std::string p = prefix_.empty() ? "" : prefix_ + "/";

    binder.Bind(p + "Position", &worldTransform_.translation_, worldTransform_.translation_, 0.1f, 0.0f, 0.0f,
        [this]() { UpdateViewMatrix(); });

    binder.Bind(p + "RotationEuler", &worldTransform_.rotation_, worldTransform_.rotation_, 0.1f, 0.0f, 0.0f,
        [this]() {
            Quaternion q = Quaternion::QuaternionFromEuler(worldTransform_.rotation_);
            worldTransform_.SetRotation(q);
            UpdateViewMatrix();
        });

    binder.Bind(p + "FOV", &fovY_, fovY_, 0.01f, 0.01f, 3.14f,
        [this]() { UpdateProjectionMatrix(); UpdateViewProjectionMatrix(); });

    binder.Bind(p + "NearClip", &nearClip_, nearClip_, 0.01f, 0.001f, 100.0f,
        [this]() { UpdateProjectionMatrix(); UpdateViewProjectionMatrix(); });

    binder.Bind(p + "FarClip", &farClip_, farClip_, 1.0f, 1.0f, 10000.0f,
        [this]() { UpdateProjectionMatrix(); UpdateViewProjectionMatrix(); });

    UpdateViewProjectionMatrix();
}

void Camera::DebugDraw(PropertyBinder& binder, const std::string& label)
{
#ifdef ENABLE_IMGUI
    std::string p = prefix_.empty() ? "" : prefix_ + "/";

    if (ImGui::TreeNode(label.c_str()))
    {
        binder.Draw(p + "Position", "座標 (World)");
        binder.Draw(p + "RotationEuler", "回転 (World)");
        binder.Draw(p + "FOV", "視野角 (FOV)");
        binder.Draw(p + "NearClip", "ニアクリップ");
        binder.Draw(p + "FarClip", "ファークリップ");
        ImGui::TreePop();
    }
#endif
}

}
