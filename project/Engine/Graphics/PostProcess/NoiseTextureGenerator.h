#pragma once

namespace FE
{

class Engine;

// 生成したテクスチャとそのSRVインデックスを保持する構造体
struct GeneratedTextureData
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    uint32_t srvIndex = 0;
};

class NoiseTextureGenerator
{
public:
    void Initialize(Engine* engine);

    // 3D Perlinノイズを生成し、リソースとSRVインデックスを返す
    GeneratedTextureData Generate3DPerlinNoise(
        ID3D12GraphicsCommandList* cmdList,
        uint32_t width, uint32_t height, uint32_t depth);

private:
    Engine* engine_ = nullptr;
};

}