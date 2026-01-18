#pragma once
#include "Vector.h"
#include "Camera.h"
#include "LightManager.h"
#include "MaterialManager.h"
#include "TextureManager.h"
#include "PostEffectManager.h"
#include "DebugCamera.h"

#include <chrono>

class Engine;

class DebugGuiManager
{
public:
    void Initialize(Engine* engine, LightManager* lightManager, MaterialManager* materialManager, 
        TextureManager* textureManager, PostEffectManager* postEffectManager, DebugCamera* debugCamera);
    void Update(Camera* targetCamera);

    void BeginSceneView(SRVManager* srvManager, uint32_t srvIndexToShow);
    void EndSceneView();

private:
    Engine* engine_; 
    LightManager* lightManager_;
    MaterialManager* materialManager_;
    TextureManager* textureManager_;
    PostEffectManager* postEffectManager_;
    DebugCamera* debugCamera_;

    // 各種パラメータを保持する変数
    // 光源設定
    Vector3 directionalLightDirection_ = { 0.0f, -1.0f, 0.0f };
    Vector4 directionalLightColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float directionalLightIntensity_ = 1.0f;

    void DrawRenderSettings();
    void DrawCameraSettings(Camera* targetCamera);
    void DrawLightSettings();
    void DrawPostEffectSettings();
    void DrawTimeSettings();
    void DrawInformationDisplays();  
};