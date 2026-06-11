#pragma once
#include "PSOManager.h"
#include "Structures.h"

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

    // 実行（5つのCSを順番にDispatchする）
    void Execute(ID3D12GraphicsCommandList* cmdList);

    // 外部（VolumetricFogPassなど）に計算結果を渡すためのゲッター
    uint32_t GetCurrentDensitySRVIndex() const { return densitySrvIndices_[readIndex_]; }
    uint32_t GetCurrentVelocitySRVIndex() const { return velocitySrvIndices_[readIndex_]; }

    D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const { return constantBuffer_->GetGPUVirtualAddress(); }

    FluidSettings* GetSettings() const { return cbData_; }

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

    // --- 流体用 3Dリソース群 (2枚ずつ) ---
    Microsoft::WRL::ComPtr<ID3D12Resource> velocityRes_[2];
    uint32_t velocitySrvIndices_[2]{};
    uint32_t velocityUavIndices_[2]{};

    Microsoft::WRL::ComPtr<ID3D12Resource> densityRes_[2];
    uint32_t densitySrvIndices_[2]{};
    uint32_t densityUavIndices_[2]{};

    Microsoft::WRL::ComPtr<ID3D12Resource> pressureRes_[2];
    uint32_t pressureSrvIndices_[2]{};
    uint32_t pressureUavIndices_[2]{};

    // --- 1枚だけで良いリソース ---
    Microsoft::WRL::ComPtr<ID3D12Resource> divergenceRes_;
    uint32_t divergenceSrvIndex_ = 0;
    uint32_t divergenceUavIndex_ = 0;

    // 定数バッファ (b1: FluidSettings)
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    FluidSettings* cbData_ = nullptr; // ※FluidSettings構造体はEngine側で定義されている想定
};

}