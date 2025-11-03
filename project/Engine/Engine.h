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
#include "TextureManager.h"
#include "Mesh.h"
#include "LightManager.h"
#include "WorldTransform.h"
#include "Camera.h"
#include "DebugCamera.h"
#include "RenderCommon.h"
#include "debugGuiManager.h"
#include "ParticleSystem.h"
#include "CameraManager.h" 
#include "PostEffectManager.h" 
#include "AnimationLoader.h" 
#include "Renderer.h" 

#include <chrono>

constexpr int32_t kClientWidth = 1280;
constexpr int32_t kClientHeight = 720;

// デバッグ描画切り替えフラグ
constexpr bool useDebugView = true;

class Engine
{
public:
    ~Engine() {}

    // 初期化・終了
    void Initialize(Camera* camera, MaterialManager* materialManager);
    void Finalize();

    // フレーム処理
    void BeginFrame();
    void EndFrame();

    // テクスチャ読み込み
    int LoadTexture(const std::string& texturePath);
    void LoadTextureArray(const std::vector<std::string>& texturePaths);

    // ブレンドモード設定
    void SetBlendMode(BlendMode blendMode) { renderer_->currentBlendMode_ = blendMode; }

    // FPS固定処理
    void InitializeFixFPS();
    void UpdateFixFPS();

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
    std::unique_ptr<PSOManager> psoManager_;
    MaterialManager* materialManager_ = nullptr;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<SRVManager> srvManager_;
    std::unique_ptr<DSVManager> dsvManager_;
    std::unique_ptr<LightManager> lightManager_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<DebugGuiManager> debugGuiManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<CameraManager> cameraManager_;
    std::unique_ptr<PostEffectManager> postEffectManager_;
    std::unique_ptr<Renderer> renderer_;
    Camera* camera_ = nullptr;

    // DirectX関連
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    HANDLE fenceEvent_ = nullptr;

    uint32_t descriptorSizeRTV_ = 0;

    bool isWireFrame_ = false;

    uint32_t offscreenSrvIndex_;

	// ウィンドウタイトル
    static std::wstring windowTitle_;

    int kTargetFPS_ = 60; // デフォルトのターゲットFPS
    // 目標とする次のフレームの終了時刻
    std::chrono::steady_clock::time_point targetTime_;
    // 1フレームあたりの時間
    const std::chrono::microseconds frameDuration_{ 1000000 / kTargetFPS_ };
};