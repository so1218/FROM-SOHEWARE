#pragma once

#include <wrl.h>        
#include <d3d12.h>      
#include <vector>       
#include <memory>       
#include "Structures.h"  

class MaterialManager
{
public:
    ~MaterialManager();

    // マテリアルの生成
    MaterialHandle CreateMaterial(ID3D12Device* device);
    MaterialHandle CreateSimpleMaterial(ID3D12Device* device);

    // グローバル設定からすべてのマテリアルを更新
    void UpdateAllMaterialsFromGlobal();

    // ゲッター
    const std::vector<MaterialHandle>& GetMaterials() const { return materials_; }
    MaterialSettings& GetMaterialSettings() { return materialSettings_; }

private:
    std::vector<MaterialHandle> materials_;
    MaterialSettings materialSettings_;
};

