#pragma once
// ゲーム側からも使うものだけを読み込む
#include "RenderCommon.h"
#include "Camera.h"
#include "LightManager.h"
#include "ParticleSystem.h"
#include "PostEffectManager.h"
#include "RendererManager.h"
#include "RenderPipeline.h"

namespace FS
{

struct ProjectConfig;
class Window;
class FrameLimiter;
class GraphicsDevice;
class CommandManager;
class SwapChain;
class DescriptorHeapManager;
class SRVManager;
class DSVManager;
class RTVManager;
class OffscreenRTVManager;
class GlobalConstants;
class RenderContext;
class ShaderManager;
class RootSignatureManager;
class PSOManager;
class MaterialManager;
class TextureLoader;
class DebugCamera;
class DebugGuiManager;

class Engine
{
public:
    Engine();
    ~Engine();

    void Initialize(const ProjectConfig& config);
    void Finalize();

    // 毎フレーム、シーンの更新のあとに呼ぶ
    void SetSceneCamera(Camera* sceneCamera);
    void SetCameraState(const RenderCameraState& camera);

    // フレーム処理
    void BeginFrame();
    void EndFrame();

    // ゲームを終える。今のフレームを最後まで処理してから、メインループを抜ける
    void RequestQuit() { isQuitRequested_ = true; }
    bool IsQuitRequested() const { return isQuitRequested_; }

    // ゲッター
    // システム関連マネージャー
    GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_.get(); }
    CommandManager* GetCommandManager() const { return commandManager_.get(); }
    SwapChain* GetSwapChain() const { return swapChain_.get(); }
    RTVManager* GetRTVManager() const { return rtvManager_.get(); }
    OffscreenRTVManager* GetOffscreenRTVManager() const { return offscreenRTVManager_.get(); }
    RenderContext* GetRenderContext() const { return renderContext_.get(); }
    RootSignatureManager* GetRootSignatureManager() const { return rootSignatureManager_.get(); }
    PSOManager* GetPSOManager() const { return psoManager_.get(); }
    MaterialManager* GetMaterialManager() const { return materialManager_.get(); }
    TextureLoader* GetTextureLoader() const { return textureLoader_.get(); }
    SRVManager* GetSRVManager() const { return srvManager_.get(); }
    DSVManager* GetDSVManager() const { return dsvManager_.get(); }
    LightManager* GetLightManager() const { return lightManager_.get(); }
    DebugCamera* GetDebugCamera() const { return debugCamera_.get(); }
    DebugGuiManager* GetDebugGuiManager() const { return debugGuiManager_.get(); }
    ParticleSystem* GetParticleSystem() const { return particleSystem_.get(); }
    GlobalConstants* GetGlobalConstants() const { return globalConstants_.get(); }
    RendererManager* GetRendererManager() const { return rendererManager_.get(); }
    PostEffectManager* GetPostEffectManager() const { return renderPipeline_->GetPostEffectManager(); }
    ShadowMap* GetShadowMap() const { return renderPipeline_->GetShadowMap(); }

    // 描く解像度 (ウィンドウのクライアント領域の大きさ)
    static uint32_t GetClientWidth() { return sClientWidth; }
    static uint32_t GetClientHeight() { return sClientHeight; }

    // UI は高さ 720 の画面を基準に作り、実際の解像度に合わせてこの倍率で拡大して描く
    static constexpr float kUIReferenceHeight = 720.0f;
    static float GetUIScale() { return static_cast<float>(sClientHeight) / kUIReferenceHeight; }
    // UI を置くときの画面の大きさ
    static float GetUIWidth() { return static_cast<float>(sClientWidth) / GetUIScale(); }
    static float GetUIHeight() { return kUIReferenceHeight; }
    const Matrix4x4& GetViewMatrix() const { return camera_.view; }
    const Matrix4x4& GetProjectionMatrix() const { return camera_.projection; }

private:
    void InitializePlatform(const ProjectConfig& config);
    void DecideClientSize(const ProjectConfig& config);
    void InitializeGraphics();
    void InitializeRenderer();
    void InitializeResources();
    void InitializeAudio();
    void InitializeDevelopmentTools();

private:
    static uint32_t sClientWidth;
    static uint32_t sClientHeight;

    // 現在設定されているカメラ
    RenderCameraState camera_{};

    bool isFinalized_ = false;
    bool isQuitRequested_ = false;

    // 作る順に並べる
    std::unique_ptr<Window> window_;
    std::unique_ptr<FrameLimiter> frameLimiter_;
    std::unique_ptr<GraphicsDevice> graphicsDevice_;
    std::unique_ptr<CommandManager> commandManager_;
    std::unique_ptr<SwapChain> swapChain_;
    std::unique_ptr<DescriptorHeapManager> descriptorManager_;
    std::unique_ptr<SRVManager> srvManager_;
    std::unique_ptr<DSVManager> dsvManager_;
    std::unique_ptr<RTVManager> rtvManager_;
    std::unique_ptr<OffscreenRTVManager> offscreenRTVManager_;
    std::unique_ptr<LightManager> lightManager_;
    std::unique_ptr<GlobalConstants> globalConstants_;
    std::unique_ptr<RenderContext> renderContext_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<RootSignatureManager> rootSignatureManager_;
    std::unique_ptr<PSOManager> psoManager_;
    std::unique_ptr<MaterialManager> materialManager_;
    std::unique_ptr<RenderPipeline> renderPipeline_;
    std::unique_ptr<TextureLoader> textureLoader_;
    std::unique_ptr<RendererManager> rendererManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<DebugGuiManager> debugGuiManager_;
};

}