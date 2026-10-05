#pragma once
#include "BufferManager.h"
#include "Structures.h"

namespace FE
{

// ライトの種類を表す列挙型
enum class SelectedLightType 
{
    None,
    Directional,
    Point,
    Spot
};

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
    ID3D12Resource* GetShadowDataResource() { return shadowDataResource_.Get(); }

    int GetDirectionalLightCount() const { return directionalLightCount_; }
    int GetPointLightCount() const { return pointLightCount_; }
    int GetSpotLightCount() const { return spotLightCount_; }

    // ポイントライトのスロットを要求する
    // 成功すればインデックス(0 ~ MAX-1)を、失敗すれば-1を返す
    int RequestPointLight();
    int RequestSpotLight();

    // ポイントライトのスロットを返却する
    void ReturnPointLight(int index);
    void ReturnSpotLight(int index);

    // 特定のポイントライトの位置を更新する
    void UpdatePointLightPosition(int index, const Vector3& position);
    void UpdateSpotLightTransform(int index, const Vector3& position, const Vector3& direction);

    // 特定のポイントライトのパラメータを更新する
    void UpdatePointLightProperties(int index, const Vector4& color, float intensity, float radius, float volumetricScatteringIntensity);
    void UpdateSpotLightProperties(int index, const Vector4& color, float intensity, float distance, float cosAngle, float volumetricScatteringIntensity);

    // ディレクショナルライトの行列更新
    void UpdateDirectionalLightShadowMatrix(int index, const Matrix4x4& viewProjection);

    // シャドウ用の行列を計算して更新する関数
    void UpdateShadowMatrix(int lightIndex, const Vector3& shadowTarget);
    // 毎フレーム呼ばれるCSM計算用関数（メインカメラの情報を渡す）
    void UpdateCascadedShadows(const Vector3& lightDir, const Matrix4x4& cameraView, const Matrix4x4& cameraProj, float cameraNear, float cameraFar);

    void DrawDebugLights();

    void DrawSelectedLightGizmo();

    // 選択状態のGetter/Setter
    void SetSelectedLight(SelectedLightType type, int index)
    {
        selectedLightType_ = type;
        selectedLightIndex_ = index;
    }
    SelectedLightType GetSelectedLightType() const { return selectedLightType_; }
    int GetSelectedLightIndex() const { return selectedLightIndex_; }

    const ShadowData* GetShadowData() const { return shadowData_; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> shadowDataResource_;

    DirectionalLight* directionalLightData_ = nullptr;
    PointLight* pointLightData_ = nullptr;
    SpotLight* spotLightData_ = nullptr;
    ShadowData* shadowData_ = nullptr;

    int directionalLightCount_ = MAX_DIRECTIONAL_LIGHTS;
    int pointLightCount_ = MAX_POINT_LIGHTS;
    int spotLightCount_ = MAX_SPOT_LIGHTS;

    std::queue<int> availablePointLightIndices_;
    std::queue<int> availableSpotLightIndices_;

    // 現在選択されているライトの情報
    SelectedLightType selectedLightType_ = SelectedLightType::None;
    int selectedLightIndex_ = -1;

    // ディレクショナルライトのエディタ表示用の仮想位置
    Vector3 directionalLightPositions_[MAX_DIRECTIONAL_LIGHTS];
};

}
