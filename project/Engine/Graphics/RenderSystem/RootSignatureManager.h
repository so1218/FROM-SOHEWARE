#pragma once

#include <d3d12.h>              
#include <wrl/client.h>         
#include <d3dcompiler.h>        
#include <assert.h>     
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

class RootSignatureManager
{
public:
    void Initialize(ID3D12Device* device);

    // ルートシグネチャ取得
    ID3D12RootSignature* GetRootSignature(const std::string& name);

private:
    // 指定名に応じてルートシグネチャを生成
    Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(const std::string& name);

    ID3D12Device* device_ = nullptr; 

    // 生成済みルートシグネチャのキャッシュ
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSignatureCache_;
};