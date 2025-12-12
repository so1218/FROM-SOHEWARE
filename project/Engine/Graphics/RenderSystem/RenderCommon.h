#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Structures.h"
#include "Mesh.h"
#include "AnimationData.h"
#include "BlendMode.h"

struct RenderData
{
    Mesh mesh;
    MaterialHandle materialHandle;
    Matrix4x4 worldMatrix;
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
    TransformationMatrix* mappedData = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> outlineResource;
    OutlineData* outlineMappedData = nullptr;
};

enum class RenderType
{
    Model,
    Skinning,
    Sprite,
    Trail,
    Particle,
    Grid,
    Line,
    Skybox
};

// 描画グループ（描画の順番）
enum class RenderGroup {
    Background = 0,
    Opaque,      // 不透明
    AlphaTest,       // アルファテスト
    Grid,
    Skybox,
    Transparent,     // 半透明 
    Trail,
    UI,              // UI
};

// 描画リクエスト情報
struct ModelSubmission
{
    const ModelData* modelData;      // モデルのメッシュデータ
    MaterialHandle materialHandle;   // 使用するマテリアル
    uint32_t textureHandle;          // テクスチャのSRV
    uint32_t envMapSrvHandle;        // 環境マップのSRV
    uint32_t toonRampHandle;
    uint32_t color;                  // メッシュカラー
    Matrix4x4 worldMatrix;           // ワールド行列

    BlendMode blendMode = BlendMode::kBlendModeNormal;

    // アウトライン設定
    bool enableOutline;
    float outlineWidth;
    Vector4 outlineColor;

    // 使用する定数バッファのインデックス
    size_t instanceIndex;

    // スキニング情報（スキニングしない場合は nullptr）
    const SkinCluster* skinCluster = nullptr;

    // 距離（ソート用）
    float depth;

    // 描画タイプ
    RenderType type;
    RenderGroup group;// ソート順の基準

    // 手動で順序を決めたい場合のみ使う
    int layerOrder = 0;
};