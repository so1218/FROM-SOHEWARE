#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "ShadowMap.h"

namespace FE
{

class PebbleRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // GPU上でハイトマップ・密度マップから小石を全自動生成
    void GeneratePebbles(
        const RenderEnvironment& env,
        const PebbleGenerationData& genData,
        uint32_t heightMapSrvHandle,
        uint32_t densityMapSrvHandle,
        D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress);

    // 描画実行 (※引数を std::vector<Mesh> から単一の const Mesh& に変更)
    void Draw(
        const RenderEnvironment& env,
        ShadowMap* shadowMap,
        uint32_t skyboxSrvHandle,
        uint32_t albedoSrvHandle,
        uint32_t normalSrvHandle,
        const Mesh& pebbleMesh, // ★ 単一メッシュに変更
        const PebbleMaterialData& materialData,
        const PebbleCullingData& cullingData);

private:
    static const int32_t kMaxInstances = 150000;
    static constexpr int kFrameCount = 3;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;

    // GenerationCSが生成した全小石バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> generatedPebbleBuffer_;

    Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer_[kFrameCount];

    // ★ 修正1: アップロードバッファもフレームごとに用意
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer_[kFrameCount];
    D3D12_DRAW_INDEXED_ARGUMENTS* mappedArgs_[kFrameCount] = {};

    // 定数バッファ
    // ★ 修正2: GenerationData もフレームごとに用意 (動的生成での競合防止)
    Microsoft::WRL::ComPtr<ID3D12Resource> generationDataResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];

    PebbleGenerationData* mappedGenData_[kFrameCount] = {};
    PebbleMaterialData* mappedMaterial_[kFrameCount] = {};
    PebbleCullingData* mappedCullingData_[kFrameCount] = {};

    // SRV/UAVインデックス
    uint32_t generatedSrvIndex_ = 0;
    uint32_t generatedUavIndex_ = 0;
    uint32_t outputUavIndex_[kFrameCount] = {};
    uint32_t outputSrvIndex_[kFrameCount] = {};
    uint32_t indirectUavIndex_[kFrameCount] = {};

    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;

    int currentFrameIndex_ = 0;
    uint32_t totalGeneratedCount_ = 0;
};

}