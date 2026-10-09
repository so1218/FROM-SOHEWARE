#pragma once
#include "BufferManager.h"
#include "Structures.h"

namespace FE
{

class PropertyBinder;

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

    // フレームの最初に呼び出し、アクティブなライト数を 0 にリセット
    void BeginFrame();

    // 毎フレームのライト登録
    bool SubmitPointLight(
        const Vector3& position,
        const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
        float intensity = 5.0f,
        float radius = 10.0f,
        float volumetricScatteringIntensity = 1.0f);

    bool SubmitSpotLight(
        const Vector3& position,
        const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
        float intensity = 5.0f,
        float distance = 20.0f,
        const Vector3& direction = { 0.0f, -1.0f, 0.0f },
        float cosAngle = 0.866f,
        float volumetricScatteringIntensity = 8.0f);

    // ディレクショナルライトの設定・更新
    void SetDirectionalLight(
        int index,
        const Vector3& direction,
        const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f },
        float intensity = 1.0f,
        float volumetricScatteringIntensity = 5.0f);

    void UpdateDirectionalLightShadowMatrix(int index, const Matrix4x4& viewProjection);

    // シャドウ行列計算
    void UpdateShadowMatrix(int lightIndex, const Vector3& shadowTarget);
    void UpdateCascadedShadows(
        const Vector3& lightDir,
        const Matrix4x4& cameraView,
        const Matrix4x4& cameraProj,
        float cameraNear,
        float cameraFar);
    void DrawDebugLights();
    void DrawSelectedLightGizmo();

    // シーン設定バインド
    void BindProperties(PropertyBinder& binder, const std::string& prefix = "Light");

    // GUI描画
    void DebugDraw(PropertyBinder& binder);

    // 選択状態の Getter/Setter
    void SetSelectedLight(SelectedLightType type, int index)
    {
        selectedLightType_ = type;
        selectedLightIndex_ = index;
    }
    SelectedLightType GetSelectedLightType() const { return selectedLightType_; }
    int GetSelectedLightIndex() const { return selectedLightIndex_; }

    // ゲッター群
    int GetDirectionalLightCount() const { return directionalLightCount_; }
    int GetActivePointLightCount() const { return activePointLightCount_; }
    int GetActiveSpotLightCount() const { return activeSpotLightCount_; }

    DirectionalLight* GetDirectionalLightData() { return directionalLightData_; }
    PointLight* GetPointLightData() { return pointLightData_; }
    SpotLight* GetSpotLightData() { return spotLightData_; }
    const ShadowData* GetShadowData() const { return shadowData_; }

    ID3D12Resource* GetDirectionalLightResource() const { return directionalLightResource_.Get(); }
    ID3D12Resource* GetPointLightResource() const { return pointLightResource_.Get(); }
    ID3D12Resource* GetSpotLightResource() const { return spotLightResource_.Get(); }
    ID3D12Resource* GetShadowDataResource() const { return shadowDataResource_.Get(); }

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
    int activePointLightCount_ = 0;
    int activeSpotLightCount_ = 0;

    SelectedLightType selectedLightType_ = SelectedLightType::None;
    int selectedLightIndex_ = -1;

    Vector3 directionalLightPositions_[MAX_DIRECTIONAL_LIGHTS];

    std::string prefix_;
};

}
