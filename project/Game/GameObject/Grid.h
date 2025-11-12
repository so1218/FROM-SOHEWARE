#pragma once

#include "Engine.h"
#include "GameObject.h"

class Grid : public GameObject
{
public:
    Grid(Engine* engine, Camera* camera, ModelData* modelData);
    ~Grid() override = default;

    void Initialize() override {};
    void Update() override {}
    void Draw() override;

    GameObjectType GetType() const override { return GameObjectType::Grid; }

    // Modelクラスと共通のセッター
    void SetWorldTransform(const WorldTransform& transform);
    void SetColor(uint32_t color);
    void SetCamera(Camera* camera);
    void SetTextureHandle(uint32_t handle);

    // ゲッター
    WorldTransform& GetTransform() { return transform_; }

private:
    Engine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    WorldTransform transform_;
    uint32_t color_ = 0xFFFFFFFF;
    uint32_t textureHandle_;
    MaterialHandle materialHandle_;
    ModelData* modelData_; 
};