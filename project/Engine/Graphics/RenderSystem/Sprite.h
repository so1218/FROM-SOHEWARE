#pragma once
#include "Vector2.h"
#include "WorldTransform.h"
#include "Structures.h"

namespace FS
{

class Engine;

struct SpriteMaterial
{
    int32_t enableDissolve = 0;
    float dissolveThreshold = 0.5f;
    float edgeWidth = 0.05f;
    float edgeIntensity = 2.0f;
    Vector3 edgeColor = { 1.0f, 0.5f, 0.0f };
    float emissiveIntensity = 1.0f;
};

class Sprite
{
public:
    Sprite(Engine* engine);
    void Draw();

    // セッター
    void SetTexture(const std::string& textureName);
    void SetDissolveTexture(const std::string& textureName);
    void SetColor(const Vector4& color);

    void SetPosition(const Vector2& position) { position_ = position; }
    void SetSize(const Vector2& size) { size_ = size; }
    void SetRotation(float rotation) { rotation_ = rotation; }
    void SetAnchorPoint(const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }
    void SetLayerOrder(int order) { layerOrder_ = order; }
    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }
    void SetUVTransform(const WorldTransform& uvTransform) { uvTransform_ = uvTransform; }

    // テクスチャの一部だけを描く
    void SetUVRect(const Vector4& uvRect) { uvRect_ = uvRect; }
    const Vector4& GetUVRect() const { return uvRect_; }

    // 色の直接代入
    void SetColor(uint32_t color) { color_ = color; }

    void SetEnableDissolve(bool enable) { material_.enableDissolve = enable ? 1 : 0; }
    SpriteMaterial* GetMaterial() { return &material_; }
    const SpriteMaterial* GetMaterial() const { return &material_; }

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

    void UpdateUV() { uvTransform_.UpdateMatrix(); }

    std::string& GetTextureName() { return textureName_; }
    std::string& GetDissolveTextureName() { return dissolveTextureName_; }
    bool* GetIsVisiblePtr() { return &isVisible_; }
    int* GetLayerOrderPtr() { return &layerOrder_; }
    Vector2* GetAnchorPointPtr() { return &anchorPoint_; }
    Vector4* GetUVRectPtr() { return &uvRect_; }

private:
    Engine* engine_ = nullptr;

    SpriteMaterial material_;                    

    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 size_ = { 1.0f, 1.0f };
    float rotation_ = 0.0f;
    Vector2 anchorPoint_ = { 0.0f, 0.0f };         

    uint32_t color_ = 0xFFFFFFFF;

    // テクスチャハンドル (描画用)
    uint32_t textureHandle_ = 0;
    uint32_t dissolveTextureHandle_ = 0;

    // テクスチャ名
    std::string textureName_ = "white1x1";
    std::string dissolveTextureName_ = "white1x1";

    WorldTransform uvTransform_;
    Vector4 uvRect_ = { 0.0f, 0.0f, 1.0f, 1.0f };  

    bool isVisible_ = true;
    int layerOrder_ = 0;
};

}