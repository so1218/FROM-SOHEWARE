#pragma once
#include "PSOManager.h"
#include "Structures.h"

namespace FS
{

class Engine;

class GrassFirePass
{
public:
    static constexpr uint32_t kMapResolution = 1024;
    static constexpr uint32_t kReadbackResolution = 256;

    GrassFirePass() = default;
    ~GrassFirePass();

    void Initialize(Engine* engine, PSOManager* psoManager);

    // settings の範囲・燃え方と、このフレームの火種で 1 フレーム進める
    void Execute(ID3D12GraphicsCommandList* cmdList, const GrassFireConstants& settings,
        const std::vector<GrassFireIgnition>& ignitions, uint32_t densityMapSrvIndex, bool isResetRequested);

    // 描画で読む地図と定数
    uint32_t GetCurrentSRVIndex() const { return srvIndices_[latestIndex_]; }
    D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const { return constantBuffer_->GetGPUVirtualAddress(); }

    // 1 フレーム前の、kReadbackResolution 四方の炎の強さ
    const std::vector<float>& GetReadbackValues() const { return readbackValues_; }
    // 1 フレーム前の、kReadbackResolution 四方の燃え進み具合
    const std::vector<float>& GetBurntReadbackValues() const { return burntReadbackValues_; }

private:
    void CreateTexture(ID3D12Device* device, DXGI_FORMAT format, uint32_t size, const wchar_t* name,
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource, uint32_t& srvIndex, uint32_t& uavIndex);
    void ReadBack();

    Engine* engine_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> fireMaps_[2];
    uint32_t srvIndices_[2]{};
    uint32_t uavIndices_[2]{};
    uint32_t latestIndex_ = 0;
    bool needsClear_ = true;

    // CPU へ読み戻す小さな地図と、その受け取り先
    Microsoft::WRL::ComPtr<ID3D12Resource> readbackTexture_;
    uint32_t readbackSrvIndex_ = 0;
    uint32_t readbackUavIndex_ = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer_;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT readbackFootprint_{};
    bool hasPendingReadback_ = false;
    std::vector<float> readbackValues_;
    std::vector<float> burntReadbackValues_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    GrassFireConstants* mappedConstants_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
};

}
