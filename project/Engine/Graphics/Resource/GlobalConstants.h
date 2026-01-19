#pragma once
#include "Structures.h" 
#include <wrl.h> 
#include <memory>        

class Camera;

// シーン全体で共有する定数バッファの管理
class GlobalConstants
{
public:
    // シングルトンや所有権を持つクラスとして使用
    void Initialize(ID3D12Device* device);

    // 更新
    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix,
        const Vector3& eyePos,
        float nearClip,
        float farClip
    );

    // ゲッター
    FrameData* GetFrameData() { return frameData_; }
    ID3D12Resource* GetResource() { return constantBuffer_.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer_;
    FrameData* frameData_ = nullptr;
};