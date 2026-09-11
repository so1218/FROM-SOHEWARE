#pragma once
#include "PSOManager.h"
#include "Structures.h"

namespace FE
{

class Engine;
class PSOManager;

class WorldInteractionPass
{
public:
    static constexpr uint32_t kMaxEntities = 64;

    WorldInteractionPass() = default;
    ~WorldInteractionPass();

    void Initialize(Engine* engine, PSOManager* psoManager, uint32_t width = 1024, uint32_t height = 1024);
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t terrainHeightMapSrvIndex, const Vector2& centerWorldPos);
    void UpdateEntities(const std::vector<InteractionEntity>& entities);

    // パラメータアクセス
    float GetWorldSize() const { return constantData_.worldSize; }
    InteractionConstants* GetSettings() { return &constantData_; }
    void SetConstants(const InteractionConstants& constants);

    // リソースアクセス
    uint32_t GetCurrentSRVIndex() const { return latestSrvIndex_; }
    D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const
    {
        return constantBuffer_ ? constantBuffer_->GetGPUVirtualAddress() : 0;
    }

private:
    Engine* engine_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    uint32_t width_ = 1024;
    uint32_t height_ = 1024;
    bool isFirstFrame_ = true;

    // ピンポンバッファ管理
    uint32_t frameCounter_ = 0;
    uint32_t readIndex_ = 0;
    uint32_t writeIndex_ = 1;
    uint32_t latestSrvIndex_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> interactionRes_[2];
    uint32_t interactionSrvIndices_[2]{};
    uint32_t interactionUavIndices_[2]{};

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    InteractionConstants* cbData_ = nullptr;
    InteractionConstants constantData_{};

    // StructuredBuffer
    Microsoft::WRL::ComPtr<ID3D12Resource> entityBuffer_;
    InteractionEntity* mappedEntityBuffer_ = nullptr;
    uint32_t entitySrvIndex_ = 0;

    Vector2 prevCenterWorldPos_{ 0.0f, 0.0f };
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_[2];
};

}