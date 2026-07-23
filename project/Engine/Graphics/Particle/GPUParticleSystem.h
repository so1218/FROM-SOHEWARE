#pragma once
#include <d3d12.h>
#include <wrl.h>

struct Particle {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    float life;
    float maxLife;
};

struct EmitterData {
    DirectX::XMFLOAT3 emitterPos;
    float deltaTime;
    float time;
    float padding[3]; // 16バイトアライメント用パディング
};

// 新しいGPU専用パーティクルシステム
class GPUParticleSystem {
public:
    static constexpr uint32_t kMaxParticles = 10000;

    void Initialize(ID3D12Device* device);
    void Update(ID3D12GraphicsCommandList* commandList, float deltaTime, DirectX::XMFLOAT3 emitterPos);
    void Draw(ID3D12GraphicsCommandList* commandList);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_; // StructuredBuffer (UAV & SRV)
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterBuffer_;  // ConstantBuffer

    // ※パイプラインステートやディスクリプタヒープはエンジン共通のもの、
    // または独自で保持しているものをセットしてください
    ID3D12PipelineState* computePSO_ = nullptr;
    ID3D12PipelineState* graphicsPSO_ = nullptr;
    ID3D12RootSignature* computeRootSignature_ = nullptr;
    ID3D12RootSignature* graphicsRootSignature_ = nullptr;

    float totalTime_ = 0.0f;
};