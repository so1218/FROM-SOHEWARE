#pragma once
#include "Vector.h"
#include "AssetBrowserWindow.h"

namespace FE
{

class Camera;
class LightManager;
class MaterialManager;
class TextureLoader;
class PostEffectManager;
class DebugCamera;
class Engine;
class SRVManager;

class DebugGuiManager
{
public:
    void Initialize(Engine* engine, LightManager* lightManager, MaterialManager* materialManager, 
        TextureLoader* textureLoader, PostEffectManager* postEffectManager, DebugCamera* debugCamera);
    void Update(Camera* targetCamera);

    void BeginSceneView(SRVManager* srvManager, uint32_t srvIndexToShow);
    void EndSceneView();

    // シーン側からドロップされた名前を受け取る関数
    std::string ConsumeDroppedModelName();

private:
    Engine* engine_; 
    LightManager* lightManager_;
    MaterialManager* materialManager_;
    TextureLoader* textureLoader_;
    PostEffectManager* postEffectManager_;
    DebugCamera* debugCamera_;

    // 各種パラメータを保持する変数
    // 光源設定
    Vector3 directionalLightDirection_ = { 0.0f, -1.0f, 0.0f };
    Vector4 directionalLightColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float directionalLightIntensity_ = 1.0f;

    // ドロップされたモデル名を一時的に保存しておく変数
    std::string droppedModelName_ = "";

    AssetBrowserWindow assetBrowserWindow_;

    void DrawRenderSettings();
    void DrawCameraSettings(Camera* targetCamera);
    void DrawLightSettings();
    void DrawPostEffectSettings();
    void DrawTimeSettings();
    void DrawInformationDisplays();  
};

}