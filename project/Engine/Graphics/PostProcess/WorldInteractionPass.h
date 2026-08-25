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
    WorldInteractionPass() = default;
    ~WorldInteractionPass();

    void Initialize(Engine* engine, PSOManager* psoManager, uint32_t width = 1024, uint32_t height = 1024);

    // 引数名を cameraWorldXZ から centerWorldPos に変更（(0,0) 固定時にも対応）
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t terrainHeightMapSrvIndex, const Vector2& centerWorldPos);

    void UpdateEntities(const std::vector<InteractionEntity>& entities);

    // constantData_ から直接取得するように変更（二重管理の防止）
    float GetWorldSize() const { return constantData_.worldSize; }

    InteractionConstants* GetSettings() { return &constantData_; }
    void SetConstants(const InteractionConstants& constants);

    uint32_t GetCurrentSRVIndex() const { return latestSrvIndex_; }

private:
    Engine* engine_ = nullptr;
    PSOManager* psoManager_ = nullptr;

    uint32_t width_ = 1024;
    uint32_t height_ = 1024;

    uint32_t frameCounter_ = 0;
    uint32_t readIndex_ = 0;
    uint32_t writeIndex_ = 1;

    Microsoft::WRL::ComPtr<ID3D12Resource> interactionRes_[2];
    uint32_t interactionSrvIndices_[2]{};
    uint32_t interactionUavIndices_[2]{};

    bool isFirstFrame_ = true;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    InteractionConstants* cbData_ = nullptr;
    InteractionConstants constantData_{};

    static constexpr uint32_t kMaxEntities = 64;
    Microsoft::WRL::ComPtr<ID3D12Resource> entityBuffer_;
    InteractionEntity* mappedEntityBuffer_ = nullptr;
    uint32_t entitySrvIndex_ = 0;

    Vector2 prevCenterWorldPos_{ 0.0f, 0.0f };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_[2];
    uint32_t latestSrvIndex_ = 0;
};

}