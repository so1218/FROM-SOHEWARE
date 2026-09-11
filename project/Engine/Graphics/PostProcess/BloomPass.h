#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"

namespace FE
{

class BloomPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, uint32_t w, uint32_t h, PSOManager* pso);

    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput) override;

    BloomSettings* GetSettings() { return &settings_; }

private:
    struct BloomMipLevel {
        uint32_t width;
        uint32_t height;

        // OffscreenRTVManager で生成したリソース情報
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
        uint32_t srvIndex = 0;

        // 描画設定
        D3D12_VIEWPORT viewport{};
        D3D12_RECT scissorRect{};

        // 階層ごとの定数バッファ
        Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
        BloomSettings* cbData = nullptr;
    };

    PSOManager* psoManager_ = nullptr;
    BloomSettings settings_{}; // UI調整用のマスター設定

    std::vector<BloomMipLevel> mipChain_;
    static constexpr uint32_t kMaxMipLevels = 5;
};

}