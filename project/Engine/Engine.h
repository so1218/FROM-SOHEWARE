#pragma once
#include "Window.h"
#include "SwapChain.h"
#include "RTVManager.h"
#include "DescriptorHeapManager.h"
#include "RenderContext.h"
#include "RenderCoordinator.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "MaterialManager.h"
#include "TextureLoader.h"
#include "LightManager.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "RenderCommon.h"
#include "debugGuiManager.h"
#include "ParticleSystem.h"
#include "GlobalConstants.h" 
#include "PostEffectManager.h" 
#include "AnimationLoader.h" 
#include "RendererManager.h" 
#include "FrameLimiter.h" 
#include "ShaderManager.h"
#include "ShadowMap.h"
#include "ProjectConfig.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "RenderPipeline.h"
#include "NoiseTextureGenerator.h"

namespace FE
{

class SRVManager;
class DSVManager;

class Engine
{
public:
    Engine();
    ~Engine();

    // 初期化・終了
    void Initialize(const ProjectConfig& config);
    void Finalize();

    // 毎フレーム、描画直前にGameクラスから呼ばれる
    void SetCameraState(
        const Matrix4x4& view,
        const Matrix4x4& projection,
        const Vector3& eyePos,
        float nearClip,
        float farClip
    );

    // フレーム処理
    void BeginFrame();
    void EndFrame();

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { rendererManager_->currentBlendMode_ = blendMode; }

    // ゲッター
    // DirectX関連
    ID3D12Fence* GetFence() const { return fence_.Get(); }
    HANDLE GetFenceEvent() const { return fenceEvent_; }
    ID3D12Resource* GetOffscreenDepthResource() const { return offscreenDepthResource_.Get(); }

    // システム関連マネージャー
    Window* GetWindow() const { return window_.get(); }
    GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_.get(); }
    CommandManager* GetCommandManager() const { return commandManager_.get(); }
    SwapChain* GetSwapChain() const { return swapChain_.get(); }
    RTVManager* GetRTVManager() const { return rtvManager_.get(); }
    OffscreenRTVManager* GetOffscreenRTVManager() const { return offscreenRTVManager_.get(); }
    DescriptorHeapManager* GetDescriptorManager() const { return descriptorManager_.get(); }
    RenderContext* GetRenderContext() const { return renderContext_.get(); }
    RootSignatureManager* GetRootSignatureManager() const { return rootSignatureManager_.get(); }
    ShaderManager* GetShaderManager() const { return shaderManager_.get(); }
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
    FrameLimiter* GetFrameLimiter() const { return frameLimiter_.get(); }
    RenderCoordinator* GetRenderCoordinator() const { return renderPipeline_->GetRenderCoordinator(); }
    PostEffectManager* GetPostEffectManager() const { return renderPipeline_->GetPostEffectManager(); }
    ShadowMap* GetShadowMap() const { return renderPipeline_->GetShadowMap(); }

    static int32_t GetClientWidth() { return sClientWidth; }
    static int32_t GetClientHeight() { return sClientHeight; }

private:
    // 各種初期化処理
    void InitializeSystem();
    void InitializeWindow();
    void InitializeInput();
    void InitializeGraphics();
    void InitializeRenderer();
    void InitializeResources();
    void InitializeImGui();
    void InitializeAudio();

private:
    uint64_t fenceValue_ = 0; // GPU同期用フェンス値

    // DirectX関連
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenDepthResource_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    HANDLE fenceEvent_ = nullptr;

    std::wstring windowTitle_;
    int kFixedFPS_ = 60;
    static int32_t sClientWidth;
    static int32_t sClientHeight;

    // 現在設定されているカメラ行列
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Vector3 eyePos_;

    // システム関連オブジェクト
    std::unique_ptr<Window> window_;
    std::unique_ptr<GraphicsDevice> graphicsDevice_;
    std::unique_ptr<CommandManager> commandManager_;
    std::unique_ptr<SwapChain> swapChain_;
    std::unique_ptr<RTVManager> rtvManager_;
    std::unique_ptr<OffscreenRTVManager> offscreenRTVManager_;
    std::unique_ptr<DescriptorHeapManager> descriptorManager_;
    std::unique_ptr<RenderContext> renderContext_;
    std::unique_ptr<RootSignatureManager> rootSignatureManager_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<PSOManager> psoManager_;
    std::unique_ptr<MaterialManager> materialManager_;
    std::unique_ptr<TextureLoader> textureLoader_;
    std::unique_ptr<SRVManager> srvManager_;
    std::unique_ptr<DSVManager> dsvManager_;
    std::unique_ptr<LightManager> lightManager_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<DebugGuiManager> debugGuiManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<GlobalConstants> globalConstants_;
    std::unique_ptr<RendererManager> rendererManager_;
    std::unique_ptr<FrameLimiter> frameLimiter_;
    std::unique_ptr<RenderPipeline> renderPipeline_;
    std::unique_ptr<NoiseTextureGenerator> noiseTextureGenerator_;
};

}