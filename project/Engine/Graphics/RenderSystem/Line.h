#pragma once
#include "WorldTransform.h"
#include "MathUtils.h"

namespace FE
{

class Engine;
class Camera;

class Line
{
public:
    Line(Engine* engine);

    void Initialize();
    void Update();
    void Draw();

    // セッター
    void SetStart(const Vector3& start) { localStart_ = start; }
    void SetEnd(const Vector3& end) { localEnd_ = end; }
    void SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); } 
    void SetColor(uint32_t color) { color_ = color; }

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }
    uint32_t* GetColorPtr() { return &color_; }

private:
    Engine* engine_ = nullptr;
    WorldTransform transform_; 

    // ローカル座標での始点と終点
    Vector3 localStart_ = { 0.0f, 0.0f, 0.0f };
    Vector3 localEnd_ = { 0.0f, 0.0f, 1.0f };

    uint32_t color_ = 0xFFFFFFFF;
};

}