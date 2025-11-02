#pragma once

#include <d3d12.h>              
#include <wrl/client.h>         
#include <d3dcompiler.h>        
#include <assert.h>             

class RootSignatureManager
{
public:
    // ルートシグネチャを全て初期化する
    void Initialize(ID3D12Device* device);

    // 各シェーダーで使用するルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature3D_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureSkinning_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureLine_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureParticles_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignaturePostProcess_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureFullScreen_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureDepthExtract_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureSkybox_;

private:
    ID3D12Device* device_ = nullptr;

    // 3dパイプライン用のルートシグネチャを生成
    void Create3dRootSignature();

    // スキニング3d描画用のルートシグネチャを生成
    void CreateSkinningRootSignature();

    // ライン描画用のルートシグネチャを生成
    void CreateLineRootSignature();

    // パーティクル描画用のルートシグネチャを生成
    void CreateParticleGraphicsRootSignature();

    void CreatePostEffectPassRootSignature();
    void CreateFullScreenRootSignature();
    void CreateDepthExtractRootSignature();
    void CreateSkyboxRootSignature();
};