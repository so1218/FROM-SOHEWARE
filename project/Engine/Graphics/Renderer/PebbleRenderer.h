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

    // CSを用いてGPU上でハイトマップ/密度マップからインスタンスを全自動生成
    void GeneratePebbles(
        const RenderEnvironment& env,
        const PebbleGenerationData& genData,
        uint32_t heightMapSrvHandle,
        uint32_t densityMapSrvHandle,
        D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress);

    void Draw(
        const RenderEnvironment& env,
        ShadowMap* shadowMap,
        uint32_t skyboxSrvHandle,
        uint32_t albedoSrvHandle,
        uint32_t normalSrvHandle,
        const Mesh& pebbleMesh,
        const PebbleMaterialData& materialData,
        const PebbleCullingData& cullingData);

private:
    static const int32_t kMaxInstances = 150000;
    static constexpr int kFrameCount = 3;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;

    // 生成後の全インスタンスを保持するマスタバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> generatedPebbleBuffer_;

    // カリング後の可視インスタンスバッファと、IndirectDraw用の引数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer_[kFrameCount];

    // CPUから初期値を流し込むためのUploadバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer_[kFrameCount];
    D3D12_DRAW_INDEXED_ARGUMENTS* mappedArgs_[kFrameCount] = {};

    // CPU-GPU非同期実行時のリソース競合を防ぐため、
    // 頻繁に更新される定数バッファ類はリングバッファ化してフレーム分確保
    Microsoft::WRL::ComPtr<ID3D12Resource> generationDataResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];

    PebbleGenerationData* mappedGenData_[kFrameCount] = {};
    PebbleMaterialData* mappedMaterial_[kFrameCount] = {};
    PebbleCullingData* mappedCullingData_[kFrameCount] = {};

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