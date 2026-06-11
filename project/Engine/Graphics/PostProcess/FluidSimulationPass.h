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

    // 初期化（解像度は 64x64x64 などが一般的です）
    void Initialize(Engine* engine, PSOManager* psoManager, UINT gridWidth = 64, UINT gridHeight = 64, UINT gridDepth = 64);

    // 実行（5つのCSを順番にDispatchする）
    void Execute(ID3D12GraphicsCommandList* cmdList);

    // 外部（VolumetricFogPassなど）に計算結果を渡すためのゲッター
    uint32_t GetCurrentDensitySRVIndex() const { return densitySrvIndices_[readIndex_]; }
    uint32_t GetCurrentVelocitySRVIndex() const { return velocitySrvIndices_[readIndex_]; }

    ID3D12Resource* GetCurrentDensityResource() const { return densityRes_[readIndex_].Get(); }

private:
    // 3Dテクスチャ（UAV/SRVのペア）を作成するヘルパー関数
    void CreateFluidTexture3D(
        ID3D12Device* device,
        DXGI_FORMAT format,
        const wchar_t* debugName,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outResource,
        uint32_t& outUavIndex,
        uint32_t& outSrvIndex
    );

    // Ping-Pongバッファのインデックスを反転させる
    void SwapBuffers() {
        std::swap(readIndex_, writeIndex_);
    }

private:
    Engine* engine_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    // シミュレーショングリッドのサイズ
    UINT width_ = 64;
    UINT height_ = 64;
    UINT depth_ = 64;

    // Ping-Pong 管理用インデックス (0 or 1)
    uint32_t readIndex_ = 0;
    uint32_t writeIndex_ = 1;

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
    FluidSettings* cbData_ = nullptr;
};

}