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
    void Initialize(Engine* engine, Camera* camera, LightManager* lightManager, MaterialManager* materialManager, 
        TextureManager* textureManager, PostEffectManager* postEffectManager, DebugCamera* debugCamera);
    void Update(); 

    void RenderOffscreenTexture(
        SRVManager* srvManager,    
        uint32_t srvIndexToShow
    );

private:
    Engine* engine_; 
    Camera* camera_;
    LightManager* lightManager_;
    MaterialManager* materialManager_;
    TextureManager* textureManager_;
    PostEffectManager* postEffectManager_;
    DebugCamera* debugCamera_;

    // 各種パラメータを保持する変数
    // カメラ設定
    float cameraFov_ = 45.0f;
    float cameraNearClip_ = 0.1f;
    float cameraFarClip_ = 1000.0f;

    // 光源設定
    Vector3 directionalLightDirection_ = { 0.0f, -1.0f, 0.0f };
    Vector4 directionalLightColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float directionalLightIntensity_ = 1.0f;

    void DrawRenderSettings();
    void DrawCameraSettings();
    void DrawLightSettings();
    void DrawPostEffectSettings();
    void DrawTimeSettings();
    void DrawInformationDisplays();  
};

// PostEffect bit flags
#define NONE                0
#define GRAYSCALE           (1 << 0)
#define SEPIA               (1 << 1)
#define PIXELATION          (1 << 2)
#define COLOR_TINT          (1 << 3)
#define VIGNETTE            (1 << 4)
#define SCREEN_NOISE        (1 << 5)
#define CHROM_ABERRATION    (1 << 6)
#define SCREEN_WAVE         (1 << 7)
#define FISHEYE             (1 << 8)
#define SCANLINE            (1 << 9)
#define BLOCK_NOISE         (1 << 10)
#define RGB_SPLIT           (1 << 11)
#define FILM_GRAIN          (1 << 12)
#define GLITCH              (1 << 13)
#define HEAT_HAZE           (1 << 14)
#define WATER_REFRACTION    (1 << 15)
