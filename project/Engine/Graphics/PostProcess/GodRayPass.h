#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{
class LightManager;

class GodRayPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);

    void Update(const Vector3& cameraPosition,
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix,
        LightManager* lightManager);

    // IPostEffect
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context
        , D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    GodRaySettings* GetSettings() { return cbData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;
    GodRaySettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
};

}