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
};

// 描画するオブジェクトの種類
enum class RenderType
{
    Model,       // 通常モデル
    Skinning,    // スキニングモデル
    Sprite,      // スプライト
    Trail,       // トレイル
    Particle,    // パーティクル
    Line,        // ライン
    Skybox,      // スカイボックス
	Grass        // 草
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
    uint32_t meshIndex;               // このモデルの何番目のメッシュ(MeshData)を描画するか
    MaterialHandle materialHandle;    // 使用マテリアル
    uint32_t textureHandle;           // テクスチャSRV
    uint32_t envMapSrvHandle;         // 環境マップSRV
    uint32_t toonRampHandle;          // トゥーンラップ
    uint32_t dissolveTextureHandle;   // ディゾルブテクスチャ
    uint32_t normalMapHandle;		  // 法線マップテクスチャ
    uint32_t rippleTextureHandle;     // 波紋用テクスチャ
    uint32_t puddleNoiseHandle;       // 水たまり用ノイズ
    uint32_t color;                   // メッシュカラー
    Matrix4x4 worldMatrix;            // ワールド変換行列

    BlendMode blendMode = BlendMode::kBlendModeNormal;  // ブレンドモード

    // アウトライン設定
    bool enableOutline;               // 有効かどうか
    float outlineWidth;               // 幅
    Vector4 outlineColor;             // 色

    size_t instanceIndex;             // 使用する定数バッファのインデックス

    const SkinCluster* skinCluster = nullptr; // スキニング情報

    float depth = 0.0f;                      // 描画ソート用距離

    CullMode cullMode;
    DepthMode depthMode;

    RenderType type;                  // 描画タイプ
    RenderGroup group;                // 描画順グループ

    int layerOrder = 0;               // 手動ソート用（UIや重ね順）

    Vector4 instancingColor;          // instanceColor用
    Matrix4x4 wvpMatrix;             
    Matrix4x4 worldInverseTranspose;
};

struct SpriteSubmission
{
    size_t instanceIndex;             // sprites_配列の何番目を使うか
    uint32_t textureHandle;           // テクスチャ
    uint32_t dissolveTextureHandle;   // ディゾルブ用テクスチャ
    MaterialHandle materialHandle;    // マテリアル
    int layerOrder;                   // 描画順（UIソート用）
};

struct RenderSettings
{
    BlendMode blendMode = BlendMode::kBlendModeNone;
    CullMode cullMode = CullMode::Back;
    DepthMode depthMode = DepthMode::Write;
    RenderGroup renderGroup = RenderGroup::Opaque;
};

namespace RenderingPreset
{
    // 通常 (不透明・裏面カリング・Z書き込み)
    static const RenderSettings Standard =
    {
        BlendMode::kBlendModeNone, CullMode::Back, DepthMode::Write, RenderGroup::Opaque
    };

    // 草・マント (不透明・両面)
    static const RenderSettings StandardNoCull =
    {
        BlendMode::kBlendModeNone, CullMode::None, DepthMode::Write, RenderGroup::Opaque
    };

    // ガラス・水 (半透明・裏面カリング)
    static const RenderSettings Transparent =
    {
        BlendMode::kBlendModeNormal, CullMode::Back, DepthMode::ReadOnly, RenderGroup::Transparent
    };

    // 魔法陣・爆発 (加算・両面・Z書き込みなし)
    static const RenderSettings AddNoCull =
    {
        BlendMode::kBlendModeAdd, CullMode::None, DepthMode::ReadOnly, RenderGroup::Transparent
    };

    // UIや前面表示用 (カリングなし・深度無視)
    static const RenderSettings UI =
    {
        BlendMode::kBlendModeNormal, CullMode::None, DepthMode::None, RenderGroup::Transparent
    };

    // グリッド用 
    static const RenderSettings Grid =
    {
        BlendMode::kBlendModeNormal, CullMode::None, DepthMode::ReadOnly, RenderGroup::Transparent
    };
}