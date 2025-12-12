#pragma once

#include "Vector2.h"
#include "WorldTransform.h"
#include "Structures.h"

class Engine;

class Sprite
{
public:
    Sprite(Engine* engine);

    void Draw();

    // セッター
    void SetPosition(const Vector2& position);
    void SetSize(const Vector2& size);
    void SetRotation(float rotation);
    void SetColor(uint32_t color);
    void SetTextureHandle(uint32_t textureHandle);
    void SetUVTransform(const WorldTransform& uvTransform);
    void SetLayerOrder(int order);
    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }

    // ゲッター
    Vector2& GetPosition();
    Vector2& GetSize();
    float& GetRotation();
    WorldTransform& GetUVTransform();
    int GetLayerOrder() const;
    bool GetIsVisible() const { return isVisible_; }

    MaterialHandle materialHandle_;

private:
    Engine* engine_ = nullptr;

    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 size_ = { 1.0f, 1.0f };
    float rotation_ = 0.0f;
    uint32_t color_ = 0xFFFFFFFF;
    uint32_t textureHandle_ = 1;
    WorldTransform uvTransform_;
    bool isVisible_ = true;

    int layerOrder_ = 0;
};
