#pragma once

#include "Vector2.h"
#include "WorldTransform.h"

class Engine;

class Sprite 
{
public:
    Sprite(Engine* engine);

    void Draw(); // エンジンへ描画命令を渡す

    // セッター
    void SetPosition(const Vector2& position);
    void SetSize(const Vector2& size);
    void SetRotation(float rotation);
    void SetColor(uint32_t color);
    void SetTextureHandle(uint32_t textureHandle);
    void SetUVTransform(const WorldTransform& uvTransform);

    // ゲッターや public メンバでもよい
    Vector2& GetPosition();
    Vector2& GetSize();
    float& GetRotation();
    WorldTransform& GetUVTransform();

private:
    Engine* engine_ = nullptr;

    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 size_ = { 1.0f, 1.0f };
    float rotation_ = 0.0f;
    uint32_t color_ = 0xFFFFFFFF;
    uint32_t textureHandle_ = 0;

    WorldTransform uvTransform_; 
};
