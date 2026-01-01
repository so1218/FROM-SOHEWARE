#pragma once
#include "Structures.h"
#include "IPostEffect.h"
#include "BrightExtractPass.h"
#include "DownsamplePass.h"
#include "BlurPass.h"
#include "BloomCombinePass.h"
#include "DepthExtractPass.h"
#include <memory>

class Engine;

class PostEffectManager
{
public:
    ~PostEffectManager();

    void Initialize(Engine* engine, UINT width, UINT height,
        RootSignatureManager* rootSigManager, PSOManager* psoManager,
        Camera* camera, SRVManager* srvManager);

    void Update();

    // ポストエフェクト実行
    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);

    // 設定データへのアクセサ (ImGui用など)

    BrightExtractSettings* GetBrightSettings() const { return brightPass_->GetSettings(); }
    // Horizontal (横) の設定を「マスター」として返します
    BlurSettings* GetHorizontalBlurSettings() const { return horizontalBlurPass_->GetSettings(); }
    BlurSettings* GetVerticalBlurSettings() const { return verticalBlurPass_->GetSettings(); } // 縦横共通の設定なら片方でOK
    CombineSettings* GetCombineSettings() const { return combinePass_->GetSettings(); }

    // --- SRVハンドルへのアクセス (描画コマンドで使用) ---
    // 合成結果（最終画像）のSRVハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetBloomCombineSRVHandle() const { return combinePass_->GetSRVHandleGPU(); }

    // 深度抽出結果のSRVハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetDepthExtractSRVHandle() const { return depthPass_->GetSRVHandleGPU(); }

    // 深度抽出結果のSRVインデックス (解放処理などでインデックスが必要な場合)
    uint32_t GetDepthExtractSRVIndex() const { return depthPass_->GetSRVIndex(); }

    PostEffectData* GetPostEffectData() const { return postEffectData_; }

    D3D12_GPU_VIRTUAL_ADDRESS GetPostEffectDataAddress() const { return cbPostEffect_->GetGPUVirtualAddress(); }

    uint32_t GetBloomCombineSRVIndex() const {
        if (combinePass_) {
            return combinePass_->GetSRVIndex(); // CombinePassが持っているはずのSRV番号
        }
        return 0;
    }

    uint32_t GetFinalPassSRVIndex() const { return finalPassSRVIndex_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetFinalPassRTV() const { return finalPassRTVHandle_; }
    ID3D12Resource* GetFinalPassResource() const { return finalPassResource_.Get(); }

    void SetSceneDepthIndex(uint32_t index) { sceneDepthIndex_ = index; }

private:
    // 依存オブジェクト
    Engine* engine_ = nullptr;
    SRVManager* srvManager_ = nullptr;
    RootSignatureManager* rootSigManager_ = nullptr;

    // 各パス (ユニークポインタで管理)
    std::unique_ptr<DepthExtractPass> depthPass_;
    std::unique_ptr<BrightExtractPass> brightPass_;
    std::unique_ptr<DownsamplePass> downsamplePass_;
    std::unique_ptr<BlurPass> verticalBlurPass_;
    std::unique_ptr<BlurPass> horizontalBlurPass_;
    std::unique_ptr<BloomCombinePass> combinePass_;

    Microsoft::WRL::ComPtr<ID3D12Resource> cbPostEffect_;
    PostEffectData* postEffectData_ = nullptr;

    // シーン情報
    uint32_t sceneTextureIndex_ = 0;
    uint32_t sceneDepthIndex_ = 0;

    uint32_t finalPassSRVIndex_ = 0;       // ImGuiやコピー描画で使うSRV
    D3D12_CPU_DESCRIPTOR_HANDLE finalPassRTVHandle_; // 描画先として使うRTV
    Microsoft::WRL::ComPtr<ID3D12Resource> finalPassResource_; // リソース本体(バリア用)
};

