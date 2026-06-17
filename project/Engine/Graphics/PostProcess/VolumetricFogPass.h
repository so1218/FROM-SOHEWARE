#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"
#include "NoiseTextureGenerator.h"

namespace FE
{

class VolumetricFogPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, UINT w, UINT h, PSOManager* pso);
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    // 外部からのアクセス用
    VolumetricFogSettings* GetSettings() { return cbData_; }

    // ノイズデータを受け取って保持する関数
    void SetNoiseData(const GeneratedTextureData& data) { noise3DData_ = data; }

    // 外部からボリュームのリストを受け取る関数
    void SetFogVolumes(const std::vector<FogVolume>& volumes)
    {
        if (!volumeCbData_) return;

        volumeCbData_->volumeCount = Math::MyMin(static_cast<uint32_t>(volumes.size()), static_cast<uint32_t>(MAX_FOG_VOLUMES));
        for (uint32_t i = 0; i < volumeCbData_->volumeCount; ++i) 
        {
            volumeCbData_->volumes[i] = volumes[i];
        }
    }

    // エディタで管理するための構造体
    struct FogVolumeData
    {
        int type = 0; // 0: Sphere, 1: Box
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Vector3 rotation = { 0.0f, 0.0f, 0.0f };
        Vector3 scale = { 10.0f, 10.0f, 10.0f };

        Vector3 color = { 1.0f, 1.0f, 1.0f };
        float density = 1.0f;

        Vector3 noiseScale = { 0.1f, 0.1f, 0.1f };
        float noiseIntensity = 0.5f;

        Vector3 windDirection = { 1.0f, 0.0f, 0.0f }; // 風の向き
        float windSpeed = 0.2f;                       // 流れる速度
        float anisotropy = 0.7f;                      // 光の筋

        float blendDistance = 0.2f; // 割合(0.0~1.0)

        // ★新ノイズ合成用のパラメータ（旧 noiseThreshold は削除）
        float coverage = 0.5f;       // 霧の量 (0.0~1.0)
        float worleyWeight = 0.5f;   // 雲の塊感/モコモコ感 (0.0~1.0)
        float erosion = 0.2f;        // 削り取り強度 (0.0~1.0)
        float noiseFeather = 0.3f;   // 境界のボケ具合 (0.001~2.0)

        // ★新規拡張パラメータ（プロ仕様）
        float distortionAmount = 0.15f; // ほどよく流体っぽくウネる歪み強さ
        float densityOffset = 0.0f;  // 密度の底上げ（マイナスにすると千切れ雲になります）
        float noiseContrast = 1.0f;  // コントラスト等倍（上げるとパキパキした形状になります）
        float heightFalloff = 0.0f;  // 高さ減衰（0.0でボリューム全体に霧が満ちる）

        bool isVisible = true;
    };

    std::vector<FogVolumeData>& GetFogVolumesData() { return editorVolumes_; }

private:
    // --- Froxel用の中間リソース ---
    // 1. 各セルの光と密度 (Injection用)
    Microsoft::WRL::ComPtr<ID3D12Resource> voxelInjectRes_;
    // 2. 蓄積された光と透過率 (Accumulation用)
    Microsoft::WRL::ComPtr<ID3D12Resource> voxelAccumulateRes_;

    Microsoft::WRL::ComPtr<ID3D12Resource> voxelInjectFilteredRes_;

    // 各パス用のUAV/SRVインデックス（SRVManagerから取得したもの）
    uint32_t injectUavIndex_;
    uint32_t injectSrvIndex_;
    uint32_t accumUavIndex_;
    uint32_t accumSrvIndex_;
    uint32_t filteredUavIndex_;
    uint32_t filteredSrvIndex_;

    // 設定用
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    VolumetricFogSettings* cbData_ = nullptr;
    PSOManager* psoManager_ = nullptr;
    GeneratedTextureData noise3DData_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

    // Froxelの解像度（画面の1/8〜1/16程度が一般的）
    const uint32_t froxelW = 160;
    const uint32_t froxelH = 90;
    const uint32_t froxelD = 64;

    // --- テンポラル用リソース ---
    // Resolveの結果を保存する2Dテクスチャ（2枚）
    Microsoft::WRL::ComPtr<ID3D12Resource> history3DRes_[2];
    uint32_t historySrvIndices_[2];
    uint32_t historyUavIndices_[2];
    Microsoft::WRL::ComPtr<ID3D12Resource> resolveOutputRes_;
    uint32_t resolveOutputSrvIndex_;
    uint32_t resolveOutputUavIndex_;

    uint32_t frameCounter_ = 0; // フレーム入れ替え用

    // 【追加】配置式フォグ用のリソースとポインタ
    Microsoft::WRL::ComPtr<ID3D12Resource> volumeConstantBuffer_;
    FogVolumeBuffer* volumeCbData_ = nullptr;

    // 【追加】エディタで編集する用の生データ配列
    std::vector<FogVolumeData> editorVolumes_;

};

}