#pragma once
#include "WorldTransform.h"
#include "Structures.h"

class Engine;

class GrassSystem
{
public:
    // コンストラクタ
    GrassSystem(Engine* engine, const std::string& modelName, const std::string& textureName);
    ~GrassSystem() = default;

    // インスタンス管理

    // 草を1本追加
    void AddGrass(const Vector3& position, const Vector3& rotation = { 0,0,0 }, const Vector3& scale = { 1,1,1 }, const Vector4& color = { 1,1,1,1 });
    void AddGrass(const WorldTransform& transform, const Vector4& color = { 1,1,1,1 });

    // 配置した草をすべてリセット
    void Clear();

    // 毎フレームの描画登録
    void Draw();

    // パラメータ設定 (マテリアル)
    void SetTexture(const std::string& textureName);
    void SetColor(const Vector4& color);

    // 風の設定
    void SetWindSpeed(float speed);
    void SetWindAmplitude(float amplitude);

    // 質感の設定
    void SetNormalBlend(float blend);
    void SetTranslucency(float translucency);
    void SetRootAO(float ao);
    void SetAlphaCutoff(float cutoff);

    // 影の設定
    void SetEnableShadow(bool enable);

    // パラメータのポインタ取得（ImGui等用）
    MaterialData* GetMaterialData() { return &materialData_; }

private:
    Engine* engine_ = nullptr;

    // 草専用のマテリアルデータ
    MaterialData materialData_{};
    uint32_t textureHandle_ = 0;

    // 草のインスタンス情報
    struct Instance 
    {
        Matrix4x4 worldMatrix;
        Vector4 color;
    };
    std::vector<Instance> instances_;
}; 
