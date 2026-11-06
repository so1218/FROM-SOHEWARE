#pragma once
#include "IPostEffect.h"
#include "PSOManager.h"
#include "NoiseTextureGenerator.h"

namespace FS
{

class VolumetricFogPass : public IPostEffect
{
public:
    void Initialize(Engine* engine, uint32_t width, uint32_t height, PSOManager* psoManager);
    void Execute(ID3D12GraphicsCommandList* cmdList, const PostEffectContext& context,
        D3D12_GPU_DESCRIPTOR_HANDLE overrideInput = { 0 }) override;

    // 置いた霧の数を 0 に戻す。毎フレーム最初に呼ぶ
    void BeginFrame();

    VolumetricFogSettings* GetSettings() { return settings_; }

    // エディタで編集する、置いた霧の設定
    struct FogVolumeData
    {
        int type = 0; // 0: 球, 1: 箱
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Vector3 rotation = { 0.0f, 0.0f, 0.0f };
        Vector3 scale = { 10.0f, 10.0f, 10.0f };

        Vector3 color = { 1.0f, 1.0f, 1.0f }; // 散乱色
        float density = 1.0f;

        Vector3 noiseScale = { 0.1f, 0.1f, 0.1f };
        float noiseIntensity = 0.5f;

        Vector3 windDirection = { 1.0f, 0.0f, 0.0f };
        float windSpeed = 0.2f;

        float blendDistance = 0.2f;

        float coverage = 0.5f;
        float erosion = 0.2f;
        float noiseFeather = 0.3f;

        float distortionAmount = 0.15f;
        float heightFalloff = 0.0f;

        bool isVisible = true;
    };

    // 置いた霧を、このフレームの描画に加える。上限を超えたら false
    bool SubmitFogVolume(const FogVolumeData& volumeData);

    std::vector<FogVolumeData>& GetFogVolumesData() { return editorVolumes_; }

private:
    // フロクセルの数。横と縦は画面の比率に合わせ、奥行きは指数で分ける
    static constexpr uint32_t kFroxelWidth = 160;
    static constexpr uint32_t kFroxelHeight = 90;
    static constexpr uint32_t kFroxelDepth = 64;

    // 霧の濃淡に使う3Dノイズの1辺の大きさ
    static constexpr uint32_t kNoiseResolution = 64;

    struct FroxelTexture
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t srvIndex = 0;
        uint32_t uavIndex = 0;
    };

    FroxelTexture CreateFroxelTexture(ID3D12Device* device, D3D12_RESOURCE_STATES initialState, const wchar_t* name);

    PSOManager* psoManager_ = nullptr;
    GeneratedTextureData noiseTexture_;

    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    VolumetricFogSettings* settings_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> passHeap_;

    FroxelTexture injectTexture_;      // 各フロクセルの光と濃さ
    FroxelTexture filteredTexture_;    // となりとならしたもの
    FroxelTexture historyTextures_[2]; // 前のフレームと混ぜたもの。交互に読み書きする
    FroxelTexture accumulateTexture_;  // 手前から積み上げたもの

    uint32_t frameCounter_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> volumeConstantBuffer_;
    FogVolumeBuffer* volumeBuffer_ = nullptr;
    std::vector<FogVolumeData> editorVolumes_;
};

}
