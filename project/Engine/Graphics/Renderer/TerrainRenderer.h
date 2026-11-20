#pragma once
#include "RenderCommon.h"
#include "RenderEnvironment.h"
#include "Structures.h"

namespace FS
{

class ShadowMap;

// 地形1つを描くのに必要なもの
struct TerrainDrawData
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    uint32_t indexCount = 0;
    const TerrainMaterial* material = nullptr;
    uint32_t heightMapHandle = 0;
    TerrainSettings settings{};
};

// 地形を、見えているチャンクの数だけインスタンス描画
class TerrainRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    void Submit(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks);
    // 遠景の地形
    void SubmitVista(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks);

    void Draw(const RenderEnvironment& env, ShadowMap* shadowMap);
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex);

    // 草や植物の生成でも、地形と同じ高さを求めるのに使う
    ID3D12Resource* GetSettingsBuffer() const { return main_.settingsBuffer.Get(); }

private:
    static constexpr uint32_t kMaxChunkCount = 4096;

    // 1 つの地形を描くのに使うバッファ
    struct Batch
    {
        bool isSubmitted = false;
        TerrainDrawData drawData;
        uint32_t chunkCount = 0;

        Microsoft::WRL::ComPtr<ID3D12Resource> settingsBuffer;
        TerrainSettings* mappedSettings = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> chunkBuffer;
        TerrainChunkData* mappedChunks = nullptr;
    };

    static void InitializeBatch(const RenderEnvironment& env, Batch& batch);
    static void SubmitBatch(Batch& batch, const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks);
    static void DrawBatch(const RenderEnvironment& env, ShadowMap* shadowMap, const Batch& batch, const char* eventName);

    Batch main_;
    Batch vista_;
};

}
