#pragma once
#include "Mesh.h"
#include "RenderCommon.h"
#include "ShadowMap.h"
#include "RenderEnvironment.h"

namespace FE
{

class GrassRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    // ★追加: GPU上でハイトマップ・密度マップから草を全自動生成する
    void GenerateGrass(
        const RenderEnvironment& env,
        const GrassGenerationData& genData,
        uint32_t heightMapSrvHandle,
        uint32_t densityMapSrvHandle,
        D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddress);

    // 毎フレームの描画実行（CullingCS -> ExecuteIndirect）
    void Draw(
        const RenderEnvironment& env,
        uint32_t windMapTextureHandle,
        ShadowMap* shadowMap,
        const GrassMaterialData& materialData,
        const GrassCullingData& cullingData);

private:
    static const int32_t kMaxInstances = 1500000;
    static constexpr int kFrameCount = 3;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cullingHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> generationHeap_;

    // [GPU内保持] GenerationCSが生成した全草の基本バッファ (Default Heap)
    Microsoft::WRL::ComPtr<ID3D12Resource> generatedGrassBuffer_;

    // [出力] CullingCSが生き残った草を書き込むバッファ (Default Heap)
    Microsoft::WRL::ComPtr<ID3D12Resource> outputInstanceBuffer_[kFrameCount];

    // [間接描画引数] CullingCSがカウントアップするバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsBuffer_[kFrameCount];

    // [リセット用] 間接描画引数を初期化するためのアップロードバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> indirectArgsUploadBuffer_;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> generationDataResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kFrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> cullingDataResource_[kFrameCount];

    GrassGenerationData* mappedGenData_ = nullptr;
    GrassMaterialData* mappedMaterial_[kFrameCount] = {};
    GrassCullingData* mappedCullingData_[kFrameCount] = {};

    // SRV/UAVインデックス
    uint32_t generatedSrvIndex_;
    uint32_t generatedUavIndex_;
    uint32_t outputUavIndex_[kFrameCount];
    uint32_t outputSrvIndex_[kFrameCount];
    uint32_t indirectUavIndex_[kFrameCount];

    // 間接描画コマンドシグネチャ
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;

    int currentFrameIndex_ = 0;
    uint32_t totalGeneratedCount_ = 0; // GPU側で生成された草の総数
};

}