#pragma once   
#include "Structures.h"  

namespace FE
{

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

}

