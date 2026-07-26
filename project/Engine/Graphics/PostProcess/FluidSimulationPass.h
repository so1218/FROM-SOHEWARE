#pragma once
#include "PSOManager.h"
#include "Structures.h"
#include "NoiseTextureGenerator.h"

namespace FE
{

class Engine;
class PSOManager;

class FluidSimulationPass
{
public:
    FluidSimulationPass() = default;
    ~FluidSimulationPass();

    // 初期化
    void Initialize(Engine* engine, PSOManager* psoManager, UINT gridWidth = 64, UINT gridHeight = 64, UINT gridDepth = 64);

    // 実行
    void Execute(ID3D12GraphicsCommandList* cmdList);

    // 外部に計算結果を渡すためのゲッター
    uint32_t GetCurrentDensitySRVIndex() const { return densitySrvIndices_[readIndex_]; }
    uint32_t GetCurrentVelocitySRVIndex() const { return velocitySrvIndices_[readIndex_]; }

    D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const { return constantBuffer_->GetGPUVirtualAddress(); }

    FluidSettings* GetSettings() const { return cbData_; }

    uint32_t GetCurrentUVWSRVIndex() const { return uvwSrvIndices_[readIndex_]; }

    // ノイズデータを受け取って保持する関数
    void SetNoiseData(const GeneratedTextureData& data) { noise3DData_ = data; }

private:
    Engine* engine_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    // シミュレーショングリッドのサイズ
    UINT width_ = 64;
    UINT height_ = 64;
    UINT depth_ = 64;

    // フレーム進行とPing-Pong管理用
    uint32_t frameCounter_ = 0;
    uint32_t readIndex_ = 0;
    uint32_t writeIndex_ = 1;

    // このパス専用のディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_[2];

    // 流体用 3Dリソース群 (2枚ずつ)
    Microsoft::WRL::ComPtr<ID3D12Resource> velocityRes_[2];
    uint32_t velocitySrvIndices_[2]{};
    uint32_t velocityUavIndices_[2]{};
    // 2-Pass Vorticity 用の一時バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> curlRes_;
    uint32_t curlSrvIndex_ = 0;
    uint32_t curlUavIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> densityRes_[2];
    uint32_t densitySrvIndices_[2]{};
    uint32_t densityUavIndices_[2]{};

    Microsoft::WRL::ComPtr<ID3D12Resource> pressureRes_[2];
    uint32_t pressureSrvIndices_[2]{};
    uint32_t pressureUavIndices_[2]{};

    // 1枚だけで良いリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> divergenceRes_;
    uint32_t divergenceSrvIndex_ = 0;
    uint32_t divergenceUavIndex_ = 0;

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    FluidSettings* cbData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> uvwRes_[2];
    uint32_t uvwUavIndices_[2];
    uint32_t uvwSrvIndices_[2];

    int previousGridX_ = 0;
    int previousGridY_ = 0;
    int previousGridZ_ = 0;

    // ノイズテクスチャ保持用
    GeneratedTextureData noise3DData_;
};

}