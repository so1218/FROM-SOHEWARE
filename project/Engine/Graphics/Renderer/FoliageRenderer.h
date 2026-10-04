#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "ShadowMap.h"

namespace FE
{

struct FoliageTypeConfig
{
    const Mesh* mesh;
    uint32_t albedoSrvHandle;
    uint32_t densityMapSrvHandle;

    FoliageMaterialData material;
    FoliageGenerationData genData;
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
        uint32_t terrainWidth, uint32_t terrainDepth);

    void Draw(
        const RenderEnvironment& env,
        ShadowMap* shadowMap,
        const FoliageCullingData& cullingData,
        D3D12_GPU_VIRTUAL_ADDRESS interactionCBAddress, 
        D3D12_GPU_DESCRIPTOR_HANDLE interactionSrvHandle);

    void UpdateConfigs(const std::vector<FoliageTypeConfig>& configs);

private:
    static constexpr int32_t kMaxInstances = 200000;
    // CPU-GPU間の同期によるストールを隠蔽するためのトリプルバッファリング
    static constexpr int kFrameCount = 3;

    struct TypeResource
    {
        FoliageTypeConfig config;

        // Generationパス用リソース
        // CS内でAppendStructuredBufferとして使用し、地形ベースでインスタンスを動的生成・追加
        Microsoft::WRL::ComPtr<ID3D12Resource> generatedBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> appendCounterBuffer;
        uint32_t generatedSrvIndex = 0;
        uint32_t generatedUavIndex = 0;

        // 生成されたインスタンスの総数を後段のカリングCSへ渡すためのSRV
        uint32_t counterSrvIndex = 0;

        // Culling & 描画パス用リソース
        // CSで視錐台カリングを行い、可視インスタンスの抽出とExecuteIndirectの引数構築をGPU上で完結
        Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer[kFrameCount];

        uint32_t outputUavIndex[kFrameCount] = {};
        uint32_t indirectUavIndex[kFrameCount] = {};
        D3D12_DRAW_INDEXED_ARGUMENTS* mappedArgs[kFrameCount] = {};

        // 定数バッファもフレーム毎に分離し、CPUからの動的更新時のリソース競合を防止
        Microsoft::WRL::ComPtr<ID3D12Resource> generationDataResource[kFrameCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource[kFrameCount];
        FoliageGenerationData* mappedGenData[kFrameCount] = {};
        FoliageMaterialData* mappedMaterial[kFrameCount] = {};
    };

    std::vector<TypeResource> types_;

    Microsoft::WRL::ComPtr<ID3D12Resource> counterResetUploadBuffer_;

    // 描画時のSetDescriptorHeapsの切り替えコストをなくすため、カリング専用のヒープを事前構築
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;

    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];
    FoliageCullingData* mappedCullingData_[kFrameCount] = {};

    int currentFrameIndex_ = 0;
    bool isGenerated_ = false;
};

}