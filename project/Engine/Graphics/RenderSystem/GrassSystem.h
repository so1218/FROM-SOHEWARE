#pragma once
#include "WorldTransform.h"
#include "Structures.h"

namespace FE
{

class Engine;

class GrassSystem
{
public:
    // コンストラクタ
    GrassSystem(Engine* engine, const std::string& windMapTextureName);
    ~GrassSystem() = default;

    // インスタンス管理

    // 草を1本追加
    void AddGrass(const Vector3& position, float height, float rotationY, float width, const Vector4& color = { 1, 1, 1, 1 });

    // 配置した草をすべてリセット
    void Clear();

    // 毎フレームの描画登録
    void Draw();

    // パラメータ設定 (マテリアル)
    void SetWindMapTexture(const std::string& textureName);
    GrassMaterialData* GetMaterialData() { return &materialData_; }

private:
    Engine* engine_ = nullptr;
    GrassMaterialData materialData_{};
    uint32_t windMapTextureHandle_ = 0;

    struct Instance
    {
        Vector3 position;
        float height;
        float rotationY;
        float width;
        uint32_t packedColor; // Vector4 を uint32 に圧縮して保持
    };
    std::vector<Instance> instances_;

    // 色の圧縮用ヘルパー
    static uint32_t PackColor(const Vector4& color);
}; 

}