#pragma once
#include "Structures.h" 
#include <wrl.h> 
#include <memory>       

class Camera;

class CameraManager
{
public:
    void Initialize(ID3D12Device* device);

    // ゲッター
    FrameData* GetFrameData() { return frameData_; }
    ID3D12Resource* GetCameraResource() { return cameraResource_.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    FrameData* frameData_ = nullptr;
};