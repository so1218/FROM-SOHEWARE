#pragma once

#include <d3d12.h>              
#include <wrl/client.h>         
#include <d3dcompiler.h>        
#include <assert.h>     

using Microsoft::WRL::ComPtr;

class RootSignatureManager
{
public:
    // すべてのルートシグネチャを生成
    void Initialize(ID3D12Device* device);

    // ゲッター
    ID3D12RootSignature* Get3DRootSignature() const { return rootSignature3D_.Get(); }
    ID3D12RootSignature* GetSkinningRootSignature() const { return rootSignatureSkinning_.Get(); }
    ID3D12RootSignature* GetLineRootSignature() const { return rootSignatureLine_.Get(); }
    ID3D12RootSignature* GetParticleRootSignature() const { return rootSignatureParticles_.Get(); }
    ID3D12RootSignature* GetPostProcessRootSignature() const { return rootSignaturePostProcess_.Get(); }
    ID3D12RootSignature* GetFullScreenRootSignature() const { return rootSignatureFullScreen_.Get(); }
    ID3D12RootSignature* GetDepthExtractRootSignature() const { return rootSignatureDepthExtract_.Get(); }
    ID3D12RootSignature* GetSkyboxRootSignature() const { return rootSignatureSkybox_.Get(); }

private:

    // ルートシグネチャ生成
    void Create3dRootSignature();               // 3D 描画
    void CreateSkinningRootSignature();         // スキニング描画
    void CreateLineRootSignature();             // ライン描画
    void CreateParticleGraphicsRootSignature(); // パーティクル描画
    void CreatePostEffectPassRootSignature();   // ポストエフェクト用
    void CreateFullScreenRootSignature();       // フルスクリーン描画
    void CreateDepthExtractRootSignature();     // 深度抽出
    void CreateSkyboxRootSignature();           // スカイボックス描画

    ID3D12Device* device_ = nullptr;

    // 使用するルートシグネチャ
    ComPtr<ID3D12RootSignature> rootSignature3D_;           // 3D 描画
    ComPtr<ID3D12RootSignature> rootSignatureSkinning_;     // スキニング
    ComPtr<ID3D12RootSignature> rootSignatureLine_;         // ライン描画
    ComPtr<ID3D12RootSignature> rootSignatureParticles_;    // パーティクル描画
    ComPtr<ID3D12RootSignature> rootSignaturePostProcess_;  // フルスクリーン処理
    ComPtr<ID3D12RootSignature> rootSignatureFullScreen_;   // ポストエフェクトパス
    ComPtr<ID3D12RootSignature> rootSignatureDepthExtract_; // 深度抽出
    ComPtr<ID3D12RootSignature> rootSignatureSkybox_;       // スカイボックス
};