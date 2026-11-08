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

// 地形を、見えているチャンクの数だけインスタンス描画する
class TerrainRenderer
{
public:
    void Initialize(const RenderEnvironment& env);
    void BeginFrame();

    void Submit(const TerrainDrawData& drawData, const std::vector<TerrainChunkData>& visibleChunks);

    void Draw(const RenderEnvironment& env, ShadowMap* shadowMap);
    void DrawShadow(const RenderEnvironment& env, uint32_t cascadeIndex);

    // 草や植物の生成でも、地形と同じ高さを求めるのに使う
    ID3D12Resource* GetSettingsBuffer() const { return settingsBuffer_.Get(); }

private:
    static constexpr uint32_t kMaxChunkCount = 4096;

    bool isSubmitted_ = false;
    TerrainDrawData drawData_;
    uint32_t chunkCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> settingsBuffer_;
    TerrainSettings* mappedSettings_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> chunkBuffer_;
    TerrainChunkData* mappedChunks_ = nullptr;
};

}
