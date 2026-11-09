#include "pch.h"
#include "Engine.h"
#include "ProjectConfig.h"
#include "Window.h"
#include "FrameLimiter.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "SwapChain.h"
#include "DescriptorHeapManager.h"
#include "RTVManager.h"
#include "GlobalConstants.h"
#include "RenderContext.h"
#include "ShaderManager.h"
#include "RootSignatureManager.h"
#include "PSOManager.h"
#include "MaterialManager.h"
#include "DebugCamera.h"
#include "DebugGuiManager.h"
#include "AudioDevice.h"
#include "DebugLayerManager.h"
#include "Logger.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "ShapeGenerator.h"
#include "DSVManager.h"
#include "TimeManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include "AnimationManager.h"
#include "GlobalVariables.h"
#include "DebugDraw.h"
#include "SRVManager.h"
#include "EnvironmentManager.h"
#include "AudioPlayer.h"
#include "FontManager.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "winmm.lib")

namespace FS
{

uint32_t Engine::sClientWidth = 1280;
uint32_t Engine::sClientHeight = 720;

Engine::Engine() = default;

Engine::~Engine()
{
	Finalize();
}

void Engine::Finalize()
{
	if (isFinalized_ || !commandManager_) return;
	isFinalized_ = true;

	// GPU が使い終わるのを待ってから片付ける
	commandManager_->WaitForGPU();

	ModelManager::GetInstance().Clear();
	FontManager::GetInstance().Clear();
	EnvironmentManager::GetInstance()->Finalize();
	ImGuiManager::Finalize();
	Input::GetInstance().Finalize();
	AudioDevice::GetInstance().Finalize();

	frameLimiter_->Finalize();
	rendererManager_->Finalize();

	CloseWindow(window_->GetHwnd());
	CoUninitialize();
}

void Engine::Initialize(const ProjectConfig& config)
{
	InitializePlatform(config);
	InitializeGraphics();
	InitializeRenderer();
	InitializeResources();
	InitializeAudio();
	InitializeDevelopmentTools();

	// 時間帯と天気
	EnvironmentManager::GetInstance()->Initialize(this);
}

void Engine::SetSceneCamera(Camera* sceneCamera)
{
	RenderCameraState camera{
		sceneCamera->GetViewMatrix(),
		sceneCamera->GetProjectionMatrix(),
		sceneCamera->GetTranslation(),
		sceneCamera->GetNearClip(),
		sceneCamera->GetFarClip()
	};

#ifdef ENABLE_DEBUG_CAMERA
	// デバッグカメラはエディター表示のときだけ使う。ゲーム画面では必ずシーンのカメラ
	if (!ImGuiManager::IsGuiVisible())
	{
		debugCamera_->SetEnabled(false);
	}
	else if (Input::GetInstance().IsKeyTriggered(DIK_Y))
	{
		debugCamera_->SetEnabled(!debugCamera_->IsEnabled());
	}

	if (debugCamera_->IsEnabled())
	{
		camera.view = debugCamera_->GetViewMatrix();
		camera.projection = debugCamera_->GetProjectionMatrix();
		camera.eyePos = debugCamera_->GetCameraWorldPosition();

#ifdef ENABLE_DEBUG_DRAW
		// シーンのカメラが映している範囲
		DebugDraw::DrawFrustum(sceneCamera->GetViewProjectionMatrix(), { 1.0f, 1.0f, 0.0f, 1.0f });
#endif
	}
#endif

#ifdef ENABLE_IMGUI
	debugGuiManager_->Update(sceneCamera);
	// ギズモは実際に描いているカメラに合わせる
	ImGuiManager::SetGizmoCamera(camera.view, camera.projection);
#endif

	SetCameraState(camera);
}

void Engine::SetCameraState(const RenderCameraState& camera)
{
	camera_ = camera;

	globalConstants_->Update(camera.view, camera.projection, camera.eyePos,
		camera.nearClip, camera.farClip,
		lightManager_->GetDirectionalLightData()[0]);

	rendererManager_->SetCameraState(camera.view, camera.projection, camera.eyePos);

	Vector3 camUp = { camera.view.m[0][1], camera.view.m[1][1], camera.view.m[2][1] };
	Vector3 camFront = { camera.view.m[0][2], camera.view.m[1][2], camera.view.m[2][2] };
	AudioPlayer::GetInstance().SetListener(camera.eyePos, camFront, camUp);
}

void Engine::BeginFrame()
{
	// ImGuiのフレーム開始
	ImGuiManager::BeginFrame();

#ifdef ENABLE_IMGUI
	GlobalVariables::GetInstance()->Update();
#endif

	// ライトのカウントをリセット
	lightManager_->BeginFrame();

	// デバッグカメラ更新
	debugCamera_->Update();

	// 時間の更新
	TimeManager::GetInstance()->Update();

	// 全3D音源の音量と左右バランスを自動計算
	AudioPlayer::GetInstance().Update();

	// ポストエフェクトのパラメータ更新など
	GetPostEffectManager()->Update(camera_.view, camera_.projection, camera_.eyePos);

#ifdef ENABLE_IMGUI
	debugGuiManager_->BeginSceneView(srvManager_.get(), GetPostEffectManager()->GetFinalPassSRVIndex());
#endif
	rendererManager_->BeginFrame();
}

void Engine::EndFrame()
{
	// このフレームに登録されたライトがそろってから、位置を表示する
	lightManager_->DrawDebugLights();

	// パイプラインに描画を丸投げ
	renderPipeline_->Render(this, rendererManager_.get(), commandManager_.get(), camera_);

	// フレーム待機（システム処理）
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理
	textureLoader_->OnFrameEnd(
		renderPipeline_->GetRenderCoordinator()->GetFenceValue(),
		commandManager_->GetCompletedFenceValue());
}




void Engine::InitializePlatform(const ProjectConfig& config)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
	Logger::Instance().Initialize();
	GlobalVariables::GetInstance()->LoadFiles();

	// DPI を自分で扱う。Windows の拡大表示でウィンドウ全体が引き伸ばされてしまうため
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	DecideClientSize(config);

	window_ = std::make_unique<Window>(GetClientWidth(), GetClientHeight());
	window_->Create(config.windowTitle, config.isFullscreen);

	Input::GetInstance().Initialize(window_->GetHInstance(), window_->GetHwnd());
	TimeManager::GetInstance()->Initialize();

	frameLimiter_ = std::make_unique<FrameLimiter>(config.targetFPS);
	frameLimiter_->Initialize();
}

void Engine::DecideClientSize(const ProjectConfig& config)
{
	if (config.isFullscreen)
	{
		// フルスクリーンはメインモニターの解像度で描く
		MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
		GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);
		sClientWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		sClientHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	}
	else
	{
		// 窓は Windows の拡大率に合わせて大きくし、そのまま描く解像度にする
		const float dpiScale = static_cast<float>(GetDpiForSystem()) / USER_DEFAULT_SCREEN_DPI;
		sClientWidth = static_cast<uint32_t>(static_cast<float>(config.width) * dpiScale);
		sClientHeight = static_cast<uint32_t>(static_cast<float>(config.height) * dpiScale);
	}
}

void Engine::InitializeGraphics()
{
	// デバッグレイヤーの初期化（デバッグビルド時のみ有効）
#ifdef _DEBUG
	DebugLayerManager::Instance().Initialize();
#endif

	// GPUアダプタの選択とD3D12デバイスの作成
	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	graphicsDevice_->Initialize();

	// コマンド関連の初期化
	commandManager_ = std::make_unique<CommandManager>();
	commandManager_->Initialize(graphicsDevice_->GetDevice());

	// スワップチェーンの初期化（画面表示用のバッファ管理）
	swapChain_ = std::make_unique<SwapChain>();
	// スワップチェーンは実際のウィンドウの大きさ (フルスクリーンならモニターの解像度) で作る
	swapChain_->Initialize(window_->GetHwnd(), commandManager_->GetCommandQueue(),
		window_->GetClientWidth(), window_->GetClientHeight(), graphicsDevice_->GetDXGIFactory());

	// ディスクリプタヒープマネージャの作成
	descriptorManager_ = std::make_unique<DescriptorHeapManager>();

	// SRVマネージャの初期化
	srvManager_ = std::make_unique<SRVManager>();
	srvManager_->Initialize(graphicsDevice_->GetDevice());

	// DSVマネージャの初期化
	dsvManager_ = std::make_unique<DSVManager>();
	dsvManager_->Initialize(graphicsDevice_->GetDevice(), descriptorManager_.get(), srvManager_.get());

	// RTVマネージャの初期化
	rtvManager_ = std::make_unique<RTVManager>();
	rtvManager_->Initialize(graphicsDevice_->GetDevice(), swapChain_->GetSwapChain(), descriptorManager_.get());

	// オフスクリーンレンダーターゲットの初期化
	offscreenRTVManager_ = std::make_unique<OffscreenRTVManager>();
	offscreenRTVManager_->Initialize(graphicsDevice_->GetDevice(), srvManager_.get(), descriptorManager_.get());

	// ライトマネージャの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(graphicsDevice_->GetDevice());

	// カメラマネージャの初期化
	globalConstants_ = std::make_unique<GlobalConstants>();
	globalConstants_->Initialize(graphicsDevice_->GetDevice());
}

void Engine::InitializeRenderer()
{
	// レンダリング制御クラス初期化
	renderContext_ = std::make_unique<RenderContext>(GetClientWidth(), GetClientHeight(),
		window_->GetClientWidth(), window_->GetClientHeight());

	// シェーダ管理
	shaderManager_ = std::make_unique<ShaderManager>();
	shaderManager_->Initialize();

	// ルートシグネチャ
	rootSignatureManager_ = std::make_unique<RootSignatureManager>();
	rootSignatureManager_->Initialize(graphicsDevice_->GetDevice());

	// PSO
	psoManager_ = std::make_unique<PSOManager>();
	psoManager_->Initialize(
		graphicsDevice_->GetDevice(),
		shaderManager_.get(),
		rootSignatureManager_.get()
	);

	// モデルやポストエフェクトのマテリアルは、ここで作る定数バッファを使う
	materialManager_ = std::make_unique<MaterialManager>();

	// RenderPipelineを作成、初期化
	renderPipeline_ = std::make_unique<RenderPipeline>();
	renderPipeline_->Initialize(this);
}

void Engine::InitializeResources()
{
	// テクスチャ管理
	textureLoader_ = std::make_unique<TextureLoader>();
	textureLoader_->Initialize(
		graphicsDevice_->GetDevice(),
		commandManager_->GetCommandList(),
		srvManager_.get()
	);

	// レンダラー
	rendererManager_ = std::make_unique<RendererManager>();
	rendererManager_->Initialize(
		graphicsDevice_.get(),
		commandManager_.get(),
		psoManager_.get(),
		rootSignatureManager_.get(),
		textureLoader_.get(),
		srvManager_.get(),
		lightManager_.get(),
		globalConstants_.get(),
		materialManager_.get(),
		GetPostEffectManager(),
		renderPipeline_->GetRenderCoordinator(),
		GetClientWidth(),
		GetClientHeight(),
		GetShadowMap()
	);

	// 共通ハンドル初期化
	TextureManager::GetInstance().LoadAllTextures(this);
	ModelManager::GetInstance().LoadAllModels(this);
	AnimationManager::GetInstance()->LoadAllAnimations();

	FontManager::GetInstance().LoadFromCSV(this);

	particleSystem_ = std::make_unique<ParticleSystem>(this);
	particleSystem_->Initialize();
}

void Engine::InitializeDevelopmentTools()
{
	// ImGui は SRV のヒープを使い、デバッグ描画は RendererManager に線を送るので、描画の準備ができてから
	ID3D12DescriptorHeap* srvHeap = srvManager_->GetSRVHeap();
	ImGuiManager::Initialize(window_->GetHwnd(), graphicsDevice_->GetDevice(),
		rtvManager_->rtvDesc, swapChain_->GetSwapChainDesc(), srvHeap,
		srvHeap->GetCPUDescriptorHandleForHeapStart(),
		srvHeap->GetGPUDescriptorHandleForHeapStart()
	);

#ifdef ENABLE_DEBUG_DRAW
	DebugDraw::Initialize(rendererManager_.get());
#endif

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();

	debugGuiManager_ = std::make_unique<DebugGuiManager>();
	debugGuiManager_->Initialize(this, lightManager_.get(), materialManager_.get(), textureLoader_.get(), GetPostEffectManager(), debugCamera_.get());
}

void Engine::InitializeAudio()
{
	// XAudioエンジン
	AudioDevice::GetInstance().Initialize();
	AudioManager::Initialize();
}

}
