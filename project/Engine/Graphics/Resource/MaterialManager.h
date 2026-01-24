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

    // すべてのマテリアルのライトモードを一括変更
    void SetGlobalLightMode(int32_t mode);

    // ゲッター
    const std::vector<MaterialHandle>& GetMaterials() const { return materials_; }

private:
    std::vector<MaterialHandle> materials_;
};

