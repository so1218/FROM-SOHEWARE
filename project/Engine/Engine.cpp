#include "pch.h"
#include "Engine.h"
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
	CoUninitialize();
}

void Engine::Initialize(const ProjectConfig& config)
{
	windowTitle_ = config.windowTitle;
	targetFPS_ = config.targetFPS;
	sClientWidth = config.width;
	sClientHeight = config.height;

	materialManager_ = std::make_unique<MaterialManager>();
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();

	frameLimiter_ = std::make_unique<FrameLimiter>(targetFPS_);
	frameLimiter_->Initialize();

	InitializeSystem();
	InitializeWindow();
	InitializeInput();
	InitializeGraphics();
	InitializeRenderer();
	InitializeResources();
	InitializeImGui();
	InitializeAudio();
	debugGuiManager_ = std::make_unique<DebugGuiManager>();
	debugGuiManager_->Initialize(this, lightManager_.get(), materialManager_.get(), textureLoader_.get(), GetPostEffectManager(), debugCamera_.get());
	particleSystem_ = std::make_unique<ParticleSystem>(this);
	particleSystem_->Initialize();

	EnvironmentManager::GetInstance()->Initialize(this);
}

void Engine::Finalize()
{
	commandManager_->WaitForGPU();

	ModelManager::GetInstance().Clear();
	FontManager::GetInstance().Clear();
	EnvironmentManager::GetInstance()->Finalize();
	ImGuiManager::Finalize();
	Input::GetInstance().Finalize();
	AudioDevice::GetInstance().Finalize();

	frameLimiter_->Finalize();
	rendererManager_->Finalize();

	// srvManager_を使うクラスを先に解放
	textureLoader_.reset();     
	renderPipeline_.reset();

	srvManager_.reset();

	// リソース解放
	CloseWindow(window_->GetHwnd());
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
	// パイプラインに描画を丸投げ
	renderPipeline_->Render(this, rendererManager_.get(), commandManager_.get(), camera_);

	// フレーム待機（システム処理）
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理
	textureLoader_->OnFrameEnd(
		renderPipeline_->GetRenderCoordinator()->GetFenceValue(),
		commandManager_->GetCompletedFenceValue());
}

void Engine::InitializeSystem()
{
	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// ロガーの初期化
	Logger::Instance().Initialize();

	GlobalVariables::GetInstance()->LoadFiles();
}

void Engine::InitializeWindow()
{
	// Windowクラスのインスタンス作成
	window_ = std::make_unique<Window>(GetClientWidth(), GetClientHeight());
	// Windowの作成
	window_->Create(windowTitle_);
}

void Engine::InitializeInput()
{
	// 入力管理の初期化
	Input::GetInstance().Initialize(window_->GetHInstance(), window_->GetHwnd());

	TimeManager::GetInstance()->Initialize();
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
	swapChain_->Initialize(window_->GetHwnd(), commandManager_->GetCommandQueue(),
		GetClientWidth(), GetClientHeight(), 2, graphicsDevice_->GetDXGIFactory());

	// ディスクリプタヒープマネージャの作成
	descriptorManager_ = std::make_unique<DescriptorHeapManager>();

	// SRVマネージャの初期化
	srvManager_ = std::make_unique<SRVManager>();
	srvManager_->Initialize(graphicsDevice_->GetDevice(), 1000);

	// DSVマネージャの初期化
	dsvManager_ = std::make_unique<DSVManager>();
	dsvManager_->Initialize(graphicsDevice_->GetDevice(), descriptorManager_.get(), srvManager_.get(), 8); 

	// RTVマネージャの初期化
	rtvManager_ = std::make_unique<RTVManager>();
	rtvManager_->Initialize(graphicsDevice_->GetDevice(), swapChain_->GetSwapChain(),
		2, descriptorManager_.get());

	// オフスクリーンレンダーターゲットの初期化
	offscreenRTVManager_ = std::make_unique<OffscreenRTVManager>();
	offscreenRTVManager_->Initialize(graphicsDevice_->GetDevice(), srvManager_.get(), descriptorManager_.get(), 32);

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
	renderContext_ = std::make_unique<RenderContext>(GetClientWidth(), GetClientHeight());

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
	ModelManager::GetInstance().LoadFromCSV(this);
	AnimationManager::GetInstance()->LoadFromCSV();

	FontManager::GetInstance().LoadFromCSV(this);

	// 3Dノイズ を生成して VolumetricFog に渡す
	{
		// ローカル変数としてジェネレーターを作成
		NoiseTextureGenerator noiseGen;
		noiseGen.Initialize(this);

		// ロード処理で使っている共通のコマンドリストを取得
		ID3D12GraphicsCommandList* cmdList = commandManager_->GetCommandList();

		// コマンドリストにノイズ生成処理(CS)を積む
		GeneratedTextureData noiseData = noiseGen.Generate3DPerlinNoise(cmdList, 64, 64, 64);

		// VolumetricFogPassに直接データを渡す
		GetPostEffectManager()->GetVolumetricFogPass()->SetNoiseData(noiseData);
	}
}

void Engine::InitializeImGui()
{
	// SRVマネージャからディスクリプタヒープを取得
	ID3D12DescriptorHeap* srvHeap = srvManager_->GetSRVHeap();

	// ImGuiの初期化
	ImGuiManager::Initialize(window_->GetHwnd(), graphicsDevice_->GetDevice(),
		rtvManager_->rtvDesc, swapChain_->GetSwapChainDesc(), srvHeap,
		srvHeap->GetCPUDescriptorHandleForHeapStart(),
		srvHeap->GetGPUDescriptorHandleForHeapStart()
	);

#ifdef ENABLE_DEBUG_DRAW
	DebugDraw::Initialize(rendererManager_.get());
#endif
}

void Engine::InitializeAudio()
{
	// XAudioエンジン
	AudioDevice::GetInstance().Initialize();
	AudioManager::Initialize();
}

}
