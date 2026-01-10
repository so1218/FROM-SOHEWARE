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

// 描画するオブジェクトの種類
enum class RenderType
{
    Model,       // 通常モデル
    Skinning,    // スキニングモデル
    Sprite,      // スプライト
    Trail,       // トレイル
    Particle,    // パーティクル
    Grid,        // グリッド
    Line,        // ライン
    Skybox       // スカイボックス
};

// 描画グループ（描画順の優先度や用途で分類）
enum class RenderGroup
{
    Background = 0,   // 背景
    Opaque,           // 不透明
    AlphaTest,        // アルファテスト
    Grid,             // グリッド
    Skybox,           // スカイボックス
    Transparent,      // 半透明
    Particle,         // パーティクル
    Trail,            // トレイル
    UI                // UI
};

// 描画要求情報（レンダリング用の1オブジェクト単位データ）
struct ModelSubmission
{
    const ModelData* modelData;       // メッシュデータ
    MaterialHandle materialHandle;    // 使用マテリアル
    uint32_t textureHandle;           // テクスチャSRV
    uint32_t envMapSrvHandle;         // 環境マップSRV
    uint32_t toonRampHandle;          // トゥーンラップ
    uint32_t dissolveTextureHandle;   // ディゾルブテクスチャ
    uint32_t color;                   // メッシュカラー
    Matrix4x4 worldMatrix;            // ワールド変換行列

    BlendMode blendMode = BlendMode::kBlendModeNormal;  // ブレンドモード

    // アウトライン設定
    bool enableOutline;               // 有効かどうか
    float outlineWidth;               // 幅
    Vector4 outlineColor;             // 色

    size_t instanceIndex;             // 使用する定数バッファのインデックス

    const SkinCluster* skinCluster = nullptr; // スキニング情報

    float depth;                      // 描画ソート用距離

    RenderType type;                  // 描画タイプ
    RenderGroup group;                // 描画順グループ

    int layerOrder = 0;               // 手動ソート用（UIや重ね順）
};