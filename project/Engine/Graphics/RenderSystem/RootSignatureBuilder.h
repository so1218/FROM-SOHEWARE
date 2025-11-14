#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

using Microsoft::WRL::ComPtr;

class RootSignatureBuilder
{
public:
    RootSignatureBuilder() = default;
    ~RootSignatureBuilder() = default;

    // コピー/ムーブ禁止
    RootSignatureBuilder(const RootSignatureBuilder&) = delete;
    RootSignatureBuilder& operator=(const RootSignatureBuilder&) = delete;
    RootSignatureBuilder(RootSignatureBuilder&&) = delete;
    RootSignatureBuilder& operator=(RootSignatureBuilder&&) = delete;

    // 定数バッファビューを追加
    void AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);

    // シェーダーリソースビューを追加
    void AddSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);

    // UAVを追加
    void AddUAV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);

    // 32ビット定数を追加
    void AddConstants(UINT shaderRegister, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace = 0);

    // ディスクリプタテーブル範囲を追加
    void AddDescriptorTableRange(
        D3D12_DESCRIPTOR_RANGE_TYPE type,
        UINT baseShaderRegister,
        UINT numDescriptors,
        D3D12_SHADER_VISIBILITY visibility,
        UINT registerSpace = 0);

    // ディスクリプタテーブルを追加
    void AddDescriptorTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges, D3D12_SHADER_VISIBILITY visibility);

    // 静的サンプラーを追加
    void AddStaticSampler(
        UINT shaderRegister,
        D3D12_FILTER filter,
        D3D12_TEXTURE_ADDRESS_MODE addressModeAll,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL,
        float maxLod = D3D12_FLOAT32_MAX);

    // ルートシグネチャを生成
    ComPtr<ID3D12RootSignature> Build(
        ID3D12Device* device,
        D3D12_ROOT_SIGNATURE_FLAGS flags,
        const std::string& nameForLogging = "");

private:
    std::vector<D3D12_ROOT_PARAMETER> parameters_;                 // ルートパラメータ一覧
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;       // 静的サンプラー一覧
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRangeStorage_; // ディスクリプタ範囲ストレージ
};