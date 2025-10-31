#pragma once
#include "BufferManager.h"
#include "Structures.h"

#include <wrl.h>
#include <queue>

constexpr int MAX_DIRECTIONAL_LIGHTS = 2;
constexpr int MAX_POINT_LIGHTS = 100;
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

    // ポイントライトのスロットを要求する
    // 成功すればインデックス (0 ~ MAX-1) を、失敗すれば -1 を返す
    int RequestPointLight();

    // ポイントライトのスロットを返却する
    void ReturnPointLight(int index);

    // 特定のポイントライトの位置を更新する
    void UpdatePointLightPosition(int index, const Vector3& position);

    // 特定のポイントライトのパラメータ（色や強さなど）を更新する
    void UpdatePointLightProperties(int index, const Vector4& color, float intensity, float radius, float decay);

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

    std::queue<int> availablePointLightIndices_;
};

