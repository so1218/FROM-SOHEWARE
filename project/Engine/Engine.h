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
#include "Mesh.h"
#include "LightManager.h"
#include "WorldTransform.h"
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

constexpr int32_t kClientWidth = 1280;
constexpr int32_t kClientHeight = 720;

class Engine
{
public:
    ~Engine() {}

    // 初期化・終了
    void Initialize();
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

    // テクスチャ読み込み
    int LoadTexture(const std::string& texturePath);

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { rendererManager_->currentBlendMode_ = blendMode; }

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

public:
    uint64_t fenceValue_ = 0; // GPU同期用フェンス値

    // システム関連オブジェクト
    std::unique_ptr<Window> window_;
    std::unique_ptr<GraphicsDevice> graphicsDevice_;
    std::unique_ptr<CommandManager> commandManager_;
    std::unique_ptr<SwapChain> swapChain_;
    std::unique_ptr<RTVManager> rtvManager_;
    std::unique_ptr<OffscreenRTVManager> offscreenRTVManager_;
    std::unique_ptr<DescriptorHeapManager> descriptorManager_;
    std::unique_ptr<RenderContext> renderContext_;
    std::unique_ptr<RenderCoordinator> renderCoordinator_;
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
    std::unique_ptr<PostEffectManager> postEffectManager_;
    std::unique_ptr<RendererManager> rendererManager_;
    std::unique_ptr<FrameLimiter> frameLimiter_;
    std::unique_ptr<ShadowMap> shadowMap_;

    // DirectX関連
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> offscreenDepthResource_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    HANDLE fenceEvent_ = nullptr;

	// ウィンドウタイトル
    static std::wstring windowTitle_;
    // 固定FPS
    static int kFixedFPS_; 

    // 現在設定されているカメラ行列
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Vector3 eyePos_;

};