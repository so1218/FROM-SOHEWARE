#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Structures.h"
#include "Mesh.h"

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

};