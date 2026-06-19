#pragma once
#include "SRVManager.h"
#include "RootSignatureManager.h"

#include <cstdint>
#include <d3d12.h>

namespace FE
{ 

struct PostEffectContext
{
    // 各種マネージャー
    SRVManager* srvManager = nullptr;
    RootSignatureManager* rootSigManager = nullptr;

    // G-Buffer等のSRVインデックス
    uint32_t sceneColorSrvIndex = 0;
    uint32_t sceneDepthSrvIndex = 0;
    uint32_t normalSrvIndex = 0;
    uint32_t materialSrvIndex = 0;

    // 流体シミュレーションの結果を受け取る変数
    uint32_t fluidDensitySrvIndex = 0;
    uint32_t fluidVelocitySrvIndex = 0; // 速度用のSRVインデックス
    uint32_t fluidUVWSrvIndex = 0;
    D3D12_GPU_VIRTUAL_ADDRESS fluidSettingsCBAddress = 0;

    // CPUハンドルの取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const {
        return srvManager->GetSRVHandleCPU_ForCopying(index);
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const {
        return srvManager->GetSRVHandleGPU(index);
    }
};

}