#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "WorldTransform.h"

class Engine;
class Camera;

class Line
{
public:
    Line(Engine* engine, Camera* camera);

    void Initialize();
    void Update();
    void Draw();

    // セッター
    void SetStart(const Vector3& start) { localStart_ = start; }
    void SetEnd(const Vector3& end) { localEnd_ = end; }
    void SetColor(uint32_t color) { color_ = color; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }

private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;
    WorldTransform transform_; 

    // ローカル座標での始点と終点
    Vector3 localStart_ = { 0.0f, 0.0f, 0.0f };
    Vector3 localEnd_ = { 0.0f, 0.0f, 1.0f };

    uint32_t color_ = 0xFFFFFFFF;
};