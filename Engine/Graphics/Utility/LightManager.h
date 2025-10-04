#pragma once
#include "BufferManager.h"
#include "Structures.h"

#include <wrl.h>

constexpr int MAX_DIRECTIONAL_LIGHTS = 2;
constexpr int MAX_POINT_LIGHTS = 4;
constexpr int MAX_SPOT_LIGHTS = 2;

class LightManager
{
public:
    void Initialize(ID3D12Device* device);

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }
    PointLight* GetPointLightData() { return pointLightData_; }
    SpotLight* GetSpotLightData() { return spotLightData_; }

    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }

    int GetDirectionalLightCount() const { return directionalLightCount_; }
    int GetPointLightCount() const { return pointLightCount_; }
    int GetSpotLightCount() const { return spotLightCount_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;

    DirectionalLight* directionalLightData_ = nullptr;
    PointLight* pointLightData_ = nullptr;
    SpotLight* spotLightData_ = nullptr;

    int directionalLightCount_ = MAX_DIRECTIONAL_LIGHTS;  
    int pointLightCount_ = MAX_POINT_LIGHTS;
    int spotLightCount_ = MAX_SPOT_LIGHTS;
};

