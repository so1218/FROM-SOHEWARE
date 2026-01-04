#pragma once
#include "Structures.h" 
#include <wrl.h> 
#include <memory>        

class Camera;

// 役割: シーン全体で共有する定数バッファ(b1)の管理
class GlobalConstants
{
public:
    // シングルトンや所有権を持つクラスとして使用
    void Initialize(ID3D12Device* device);

    // 更新
    void Update(const Camera& camera);

    // ゲッター
    FrameData* GetFrameData() { return frameData_; }
    ID3D12Resource* GetResource() { return constantBuffer_.Get(); } // 名前も汎用的に

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    FrameData* frameData_ = nullptr;
};