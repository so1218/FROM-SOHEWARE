#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Structures.h"
#include "Mesh.h"
#include "AnimationData.h"

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
    Particle
};

// 描画リクエストデータ
struct ModelSubmission
{
    const ModelData* modelData;      // メッシュデータへの参照
    MaterialHandle materialHandle;   // マテリアル
    uint32_t textureHandle;          // テクスチャ
    uint32_t envMapSrvHandle;        // 環境マップ
    uint32_t color;                  // 色
    Matrix4x4 worldMatrix;           // ワールド行列

    // アウトライン情報
    bool enableOutline;
    float outlineWidth;
    Vector4 outlineColor;

    // 割り当てられた定数バッファのインデックス（後述）
    size_t instanceIndex;

    const SkinCluster* skinCluster = nullptr;

    uint32_t priority; // 描画順 (UI=100, 不透明=0, 半透明=50 など)
    float depth;       // カメラからの距離 (半透明ソート用)

    // --- 描画タイプとデータ ---
    RenderType type;

    // 各データへのポインタ（共用体 union を使うとメモリ節約になりますが、まずはポインタでOK）
    // 描画時に type を見て、適切な型にキャストして使います
    const void* data;
};