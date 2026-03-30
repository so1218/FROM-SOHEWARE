#pragma once
#include "BufferManager.h"
#include "Structures.h"

namespace FE
{

constexpr int MAX_DIRECTIONAL_LIGHTS = 2;
constexpr int MAX_POINT_LIGHTS = 100;
constexpr int MAX_SPOT_LIGHTS = 2;
constexpr int MAX_AREA_LIGHTS = 2;

class LightManager
{
public:
    void Initialize(ID3D12Device* device);

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }
    PointLight* GetPointLightData() { return pointLightData_; }
    SpotLight* GetSpotLightData() { return spotLightData_; }
    AreaLight* GetAreaLightData() { return areaLightData_; }

    ID3D12Resource* GetDirectionalLightResource() { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() { return spotLightResource_.Get(); }
    ID3D12Resource* GetAreaLightResource() { return areaLightResource_.Get(); }

    int GetDirectionalLightCount() const { return directionalLightCount_; }
    int GetPointLightCount() const { return pointLightCount_; }
    int GetSpotLightCount() const { return spotLightCount_; }
    int GetAreaLightCount() const { return areaLightCount_; }

    // ポイントライトのスロットを要求する
    // 成功すればインデックス(0 ~ MAX-1)を、失敗すれば-1を返す
    int RequestPointLight();
    int RequestAreaLight();

    // ポイントライトのスロットを返却する
    void ReturnPointLight(int index);
    void ReturnAreaLight(int index);

    // 特定のポイントライトの位置を更新する
    void UpdatePointLightPosition(int index, const Vector3& position);

    // 特定のポイントライトのパラメータを更新する
    void UpdatePointLightProperties(int index, const Vector4& color, float intensity, float radius, float decay);
    void UpdateAreaLightProperties(int index, const Vector4& color, float intensity,
        const Vector3& position, const Vector3& right, const Vector3& up,
        float range, float decay);

    // ディレクショナルライトの行列更新
    void UpdateDirectionalLightShadowMatrix(int index, const Matrix4x4& viewProjection);

    // シャドウ用の行列を計算して更新する関数
    void UpdateShadowMatrix(int lightIndex, const Vector3& shadowTarget);

    void DrawDebugLights();

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> areaLightResource_;

    DirectionalLight* directionalLightData_ = nullptr;
    PointLight* pointLightData_ = nullptr;
    SpotLight* spotLightData_ = nullptr;
    AreaLight* areaLightData_ = nullptr;

    int directionalLightCount_ = MAX_DIRECTIONAL_LIGHTS;
    int pointLightCount_ = MAX_POINT_LIGHTS;
    int spotLightCount_ = MAX_SPOT_LIGHTS;
    int areaLightCount_ = MAX_AREA_LIGHTS;

    std::queue<int> availablePointLightIndices_;
    std::queue<int> availableAreaLightIndices_;
};

}
