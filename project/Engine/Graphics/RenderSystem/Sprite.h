#pragma once

#include "Vector2.h"
#include "WorldTransform.h"
#include "Structures.h"
#include "TextureHandle.h"

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
    void SetColor(const Vector4& color) { color_ = Math::ColorVectorToUint32(color); }
    void SetTexture(TextureID id);
    void SetUVTransform(const WorldTransform& uvTransform);
    void SetLayerOrder(int order);
    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }
    void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }

    // ゲッター
    Vector2& GetPosition();
    Vector2& GetSize();
    float& GetRotation();
    uint32_t* GetColorPtr() { return &color_; }
    WorldTransform& GetUVTransform();
    int GetLayerOrder() const;
    bool GetIsVisible() const { return isVisible_; }
    const Vector2& GetAnchorPoint() const { return anchorPoint_; }

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
    // アンカーポイント (デフォルトは左上)
    Vector2 anchorPoint_ = { 0.0f, 0.0f };

    int layerOrder_ = 0;
};
