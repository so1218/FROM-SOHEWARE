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
#define INVERT_COLOR        (1 << 1)
#define SEPIA               (1 << 2)
#define BRIGHTNESS          (1 << 3)
#define POSTERIZATION       (1 << 4)
#define PIXELATION          (1 << 5)
#define COLOR_TINT          (1 << 6)
#define CONTRAST            (1 << 7)
#define SATURATION          (1 << 8)
#define HUE_SHIFT           (1 << 9)
#define CHANNEL_SWAP        (1 << 10)
#define CEL_SHADING         (1 << 11)
#define NORMAL_OUTLINE      (1 << 12)
#define BRIGHT_EXTRACT      (1 << 13)
#define VIGNETTE            (1 << 14)
#define SCREEN_NOISE        (1 << 15)
#define CHROM_ABERRATION    (1 << 16)
#define SCREEN_WAVE         (1 << 17)
#define FISHEYE             (1 << 18)
#define FLASH               (1 << 19)
#define SCANLINE            (1 << 20)
#define BLOCK_NOISE         (1 << 21)
#define SOLARIZE            (1 << 22)
#define MULTI_POSTERIZE     (1 << 23)
#define RGB_SPLIT           (1 << 24)
#define INVERT_BY_Y         (1 << 25)
#define FILM_GRAIN          (1 << 26)
#define GLITCH              (1 << 27)
#define EDGE_DETECTION      (1 << 28)
#define HEAT_HAZE           (1 << 29)
#define SPLIT_TONING        (1 << 30)
#define WATER_REFRACTION    (1 << 31)