#pragma once
#include "Vector2.h"
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;

class Sprite
{
public:
    Sprite(Engine* engine);
    void Draw();

    // 複雑なセッター
    void SetTexture(const std::string& textureName);
    void SetDissolveTexture(const std::string& textureName);
    void SetColor(const Vector4& color);

    // 単純なセッター
    void SetPosition(const Vector2& position) { position_ = position; }
    void SetSize(const Vector2& size) { size_ = size; }
    void SetRotation(float rotation) { rotation_ = rotation; }
    void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }
    void SetLayerOrder(int order) { layerOrder_ = order; }
    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }
    void SetUVTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }

    // 色の直接代入
    void SetColor(uint32_t color) { color_ = color; }

    // マテリアルパラメータ
    void SetEnableDissolve(bool enable) { materialHandle_.materialData->enableDissolve = enable; }

    // マテリアルデータへのアクセサ
    MaterialData* GetMaterial() { return materialHandle_.materialData; }
    const MaterialData* GetMaterial() const { return materialHandle_.materialData; }

    // ゲッター
    Vector2& GetPosition() { return position_; }
    const Vector2& GetPosition() const { return position_; }

    Vector2& GetSize() { return size_; }
    const Vector2& GetSize() const { return size_; }

    float& GetRotation() { return rotation_; }
    const float& GetRotation() const { return rotation_; }

    uint32_t GetColor() const { return color_; }
    uint32_t* GetColorPtr() { return &color_; }

    WorldTransform& GetUVTransform() { return uvTransform_; }
    const WorldTransform& GetUVTransform() const { return uvTransform_; }

    int GetLayerOrder() const { return layerOrder_; }
    bool GetIsVisible() const { return isVisible_; }
    const Vector2& GetAnchorPoint() const { return anchorPoint_; }

    // ImGui用
    void UpdateUV()
    {
        uvTransform_.UpdateMatrix();
        // マテリアルデータへの転送
        if (materialHandle_.materialData)
        {
            materialHandle_.materialData->uvTransform = uvTransform_.matWorld_;
        }
    }
    std::string& GetTextureName() { return textureName_; }
    std::string& GetDissolveTextureName() { return dissolveTextureName_; }
    bool* GetIsVisiblePtr() { return &isVisible_; }
    int* GetLayerOrderPtr() { return &layerOrder_; }
    Vector2* GetAnchorPointPtr() { return &anchorPoint_; }

private:
    Engine* engine_ = nullptr;

    MaterialHandle materialHandle_;

    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 size_ = { 1.0f, 1.0f };
    float rotation_ = 0.0f;
    Vector2 anchorPoint_ = { 0.0f, 0.0f }; // デフォルトは左上

    uint32_t color_ = 0xFFFFFFFF;

    // テクスチャハンドル (描画用)
    uint32_t textureHandle_ = 0;
    uint32_t dissolveTextureHandle_ = 0;

    // テクスチャハンドル
    std::string textureName_ = "white1x1";
    std::string dissolveTextureName_ = "white1x1";

    WorldTransform uvTransform_;

    bool isVisible_ = true;
    int layerOrder_ = 0;
};

}