#pragma once
#include "Structures.h"
#include "RTVManager.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "Camera.h"
#include "SRVManager.h"

class Engine;

class PostEffectManager
{
public:
    // 初期化
    void Initialize(Engine* engine, ID3D12Device* device, OffscreenRTVManager* offscreenRTVManager, UINT width, UINT height,
        RootSignatureManager* rootSignatureManager, PSOManager* psoManager, Camera* camera, SRVManager* srvManager);

    // 更新処理
    void Update();

    // ポストエフェクトモードの切替
    void SetMode(int mode);

    // 各エフェクト用定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBrightExtract_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBlur_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBloom_;

    // 深度関連リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cbDepthExtractVS_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbDepthExtractPS_;
    D3D12_GPU_DESCRIPTOR_HANDLE depthTextureSRV_;
    uint32_t depthExtractIndex_ = 0;

    PostEffectData* postEffectData_ = nullptr;

    // CPU側から書き込み可能なマッピングポインタ
    BrightExtractSettings* brightExtractData_ = nullptr;
    BlurSettings* blurSettingsData_ = nullptr;
    CombineSetting* combineSettingsData_ = nullptr;
    DepthExtractSettingsVS* depthExtractVSData_ = nullptr;
    DepthExtractSettingsPS* depthExtractPSData_ = nullptr;

    // ポストエフェクト実行
    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);
    void ExecuteNeonPostEffect(ID3D12GraphicsCommandList* cmdList);

    // 各エフェクト用SRV/RTVインデックス
    uint32_t brightExtractIndex_ = 0;
    uint32_t verticalBlurIndex_ = 0;
    uint32_t horizontalBlurIndex_ = 0;
    uint32_t bloomCombineIndex_ = 0;
    uint32_t neonIndex_ = 0;
    uint32_t sceneDepthIndex_ = 0;

private:
    // 依存オブジェクト
    Engine* engine_;
    OffscreenRTVManager* offscreenRTVManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_;
    PSOManager* psoManager_;
    SRVManager* srvManager_;

    // シーンテクスチャやSRVテーブル
    D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureSRV_;
    D3D12_GPU_DESCRIPTOR_HANDLE bloomCombineSRVTable_;

    // オフスクリーンRTVハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE brightExtractRTVHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE verticalBlurRTVHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE horizontalBlurRTVHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE bloomCombineRTVHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE neonRTVHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE depthExtractRTVHandle_;

    // SRVインデックス
    uint32_t sceneTextureSRVIndex_;

    // 各エフェクト用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> brightExtractResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> verticalBlurResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> horizontalBlurResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> bloomCombineResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> neonResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthExtractResource_;

    // SRV用ディスクリプタヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvTableHeap_;
    UINT descriptorSize_ = 0;
};

