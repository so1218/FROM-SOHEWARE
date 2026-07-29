#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>

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
    uint32_t emitCount; 
};

class GPUParticleSystem {
public:
    static constexpr uint32_t kMaxParticles = 10000;

    void Initialize(ID3D12Device* device);

    // 初回フレーム前に1度だけ呼び出す初期化関数
    void Init(ID3D12GraphicsCommandList* commandList);

    void Emit(ID3D12GraphicsCommandList* commandList, uint32_t emitCount, DirectX::XMFLOAT3 emitterPos);
    void Update(ID3D12GraphicsCommandList* commandList, float deltaTime);
    void Draw(ID3D12GraphicsCommandList* commandList);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> emitterBuffer_;

    Microsoft::WRL::ComPtr<ID3D12Resource> freeListBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListCounter_;

    ID3D12PipelineState* computePSO_Init_ = nullptr;   
    ID3D12PipelineState* computePSO_Update_ = nullptr;
    ID3D12PipelineState* computePSO_Emit_ = nullptr;
    ID3D12PipelineState* graphicsPSO_ = nullptr;
    ID3D12RootSignature* computeRootSignature_ = nullptr;
    ID3D12RootSignature* graphicsRootSignature_ = nullptr;

    float totalTime_ = 0.0f;
};