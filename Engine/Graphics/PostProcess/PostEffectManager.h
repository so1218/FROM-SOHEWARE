#pragma once
#include "Structures.h"
#include "RTVManager.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "Camera.h"

class Engine;

class PostEffectManager
{
public:
    void Initialize(Engine* engine, ID3D12Device* device, OffscreenRTVManager* offscreenRTVManager, UINT width, UINT height, RootSignatureManager* rootSignatureManager, PSOManager* psoManager, Camera* camera);
    void Update();
    void SetMode(int mode);

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    // 各エフェクト用のCB
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBrightExtract_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBlur_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbBloom_;

    // Depth関連のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cbDepthExtractVS_;
    Microsoft::WRL::ComPtr<ID3D12Resource> cbDepthExtractPS_;
    D3D12_GPU_DESCRIPTOR_HANDLE depthTextureSRV_;
    uint32_t depthExtractIndex_ = 0;

    PostEffectData* postEffectData_ = nullptr;

    // CPU 側へのマッピング用ポインタ
    BrightExtractSettings* brightExtractData_ = nullptr;
    BlurSettings* blurSettingsData_ = nullptr;
    CombineSetting* combineSettingsData_ = nullptr;
    DepthExtractSettingsVS* depthExtractVSData_ = nullptr;
    DepthExtractSettingsVS* depthExtractPSData_ = nullptr;

    void ExecutePostEffects(ID3D12GraphicsCommandList* cmdList);
    void ExecuteNeonPostEffect(ID3D12GraphicsCommandList* cmdList);

    uint32_t brightExtractIndex_ = 0;
    uint32_t verticalBlurIndex_ = 0;
    uint32_t horizontalBlurIndex_ = 0;
    uint32_t bloomCombineIndex_ = 0;
    uint32_t neonIndex_ = 0;
    uint32_t sceneDepthIndex_ = 0;
   
private:
    Engine* engine_;
    OffscreenRTVManager* offscreenRTVManager_ = nullptr;
    RootSignatureManager* rootSignatureManager_;
    PSOManager* psoManager_;
  
    D3D12_GPU_DESCRIPTOR_HANDLE sceneTextureSRV_;
    D3D12_GPU_DESCRIPTOR_HANDLE bloomCombineSRVTable_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvTableHeap_;
    UINT descriptorSize_ = 0;


};

