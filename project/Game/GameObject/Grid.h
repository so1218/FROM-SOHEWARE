#pragma once

#include "Engine.h"
#include "GameObject.h"

class Grid : public GameObject
{
public:
    Grid(Engine* engine);
    ~Grid() override = default;

    void Initialize() override {};
    void Update() override {}
    void Draw() override;

    GameObjectType GetType() const override { return GameObjectType::Grid; }

    // Modelクラスと共通のセッター
    void SetWorldTransform(const WorldTransform& transform);
    void SetColor(uint32_t color);
    void SetTextureHandle(uint32_t handle);

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }

private:

    WorldTransform transform_;
    uint32_t color_ = 0xFFFFFFFF;
    uint32_t textureHandle_;
    MaterialHandle materialHandle_;
    const ModelData* modelData_;
};