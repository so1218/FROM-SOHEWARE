#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "ShadowMap.h"

namespace FE
{

struct FoliageTypeConfig
{
    const Mesh* mesh;               // メッシュ
    uint32_t albedoSrvHandle;       // アルベドテクスチャのハンドル
    uint32_t normalSrvHandle;       // ノーマルマップのハンドル
    uint32_t densityMapSrvHandle;

    FoliageMaterialData material;   // この植物専用のマテリアルパラメータ
    FoliageGenerationData genData;  // この植物専用の生成ルール（密度、スケールなど）
};

class FoliageRenderer
{
public:

    void Initialize(const RenderEnvironment& env, const std::vector<FoliageTypeConfig>& configs);
    void BeginFrame();

    void GenerateFoliage(
        const RenderEnvironment& env,
        uint32_t heightMapSrvHandle,
        D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress,
        UINT terrainWidth, UINT terrainDepth);

    void Draw(
        const RenderEnvironment& env,
        ShadowMap* shadowMap,
        const FoliageCullingData& cullingData);

    void UpdateConfigs(const std::vector<FoliageTypeConfig>& configs);

private:
    static constexpr int32_t kMaxInstances = 200000;
    static constexpr int kFrameCount = 3;

    struct TypeResource 
    {
        // --- 設定データ ---
        FoliageTypeConfig config;

        // --- Generation パス用 ---
        Microsoft::WRL::ComPtr<ID3D12Resource> generatedBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> appendCounterBuffer;
        uint32_t generatedSrvIndex = 0;
        uint32_t generatedUavIndex = 0;

        // ★追加: カウンタバッファのSRV用インデックス
        uint32_t counterSrvIndex = 0;

        // --- Culling & Draw パス用 ---
        Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer[kFrameCount];

        uint32_t outputUavIndex[kFrameCount] = {};
        uint32_t indirectUavIndex[kFrameCount] = {};
        D3D12_DRAW_INDEXED_ARGUMENTS* mappedArgs[kFrameCount] = {};

        // --- 種類・フレームごとの定数バッファ ---
        Microsoft::WRL::ComPtr<ID3D12Resource> generationDataResource[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource[kFrameCount];
        FoliageGenerationData* mappedGenData[kFrameCount] = {};
        FoliageMaterialData* mappedMaterial[kFrameCount] = {};
    };

    // 登録された植物のリスト
    std::vector<TypeResource> types_;

    Microsoft::WRL::ComPtr<ID3D12Resource> counterResetUploadBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;

    // 全体共通の定数バッファ (カリング用など)
    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];
    FoliageCullingData* mappedCullingData_[kFrameCount] = {};

    int currentFrameIndex_ = 0;
    bool isGenerated_ = false;
};

}