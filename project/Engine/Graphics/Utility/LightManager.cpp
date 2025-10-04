#include "LightManager.h"
#include "Structures.h"

void LightManager::Initialize(ID3D12Device* device)
{
    // Directional Light
    directionalLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHTS);
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i)
    {
        directionalLightData_[i].enable = false;
        directionalLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        directionalLightData_[i].direction = { 0.0f, -1.0f, 1.25f };
        directionalLightData_[i].intensity = 1.0f;
    }

    // Point Light
    pointLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(PointLight) * MAX_POINT_LIGHTS);
    pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        pointLightData_[i].enable = false;
        pointLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        pointLightData_[i].position = { -4.0f + i * 2.0f, 5.0f, 0.0f };
        pointLightData_[i].intensity = 5.0f;
        pointLightData_[i].radius = 10.0f;
        pointLightData_[i].decay = 3.0f;
    }

    // Spot Light
    spotLightResource_ = BufferManager::CreateBufferResource(
        device, sizeof(SpotLight) * MAX_SPOT_LIGHTS);
    spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
    {
        spotLightData_[i].enable = false;
        spotLightData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
        spotLightData_[i].position = { 0.0f, 5.0f, float(i * 2) };
        spotLightData_[i].intensity = 5.0f;
        spotLightData_[i].direction = { 0.0f, -1.0f, 0.0f };
        spotLightData_[i].distance = 20.0f;
        spotLightData_[i].decay = 3.0f;
        spotLightData_[i].cosAngle = 0.866f;
    }
}