#pragma once

#include "Engine.h"

class Grid
{
public:
    Grid(Engine* engine, Camera* camera, ModelData* modelData);
    void Draw();

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
    ModelData* modelData_; // グリッド用のメッシュデータ
};