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

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "winmm.lib")

int32_t Engine::sClientWidth = 1280;
int32_t Engine::sClientHeight = 720;

Engine::Engine() = default;

Engine::~Engine()
{
	CoUninitialize();
}

void Engine::Initialize(const ProjectConfig& config)
{
	windowTitle_ = config.windowTitle;
	kFixedFPS_ = config.targetFPS;
	sClientWidth = config.width;
	sClientHeight = config.height;

	materialManager_ = std::make_unique<MaterialManager>();
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();

	frameLimiter_ = std::make_unique<FrameLimiter>(kFixedFPS_); 
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
}

void Engine::Finalize()
{
	ImGuiManager::Finalize();

	// Fence待機
	fenceValue_++;
	commandManager_->GetCommandQueue()->Signal(fence_.Get(), fenceValue_);
	if (fence_->GetCompletedValue() < fenceValue_)
	{
		fence_.Get()->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	frameLimiter_->Finalize();
	rendererManager_->Finalize();

	// srvManager_を使うクラスを先に解放
	textureLoader_.reset();     
	renderPipeline_.reset();

	srvManager_.reset();

	// リソース解放
	CloseHandle(fenceEvent_);
	CloseWindow(window_->GetHwnd());
}

void Engine::SetCameraState(
	const Matrix4x4& view,
	const Matrix4x4& projection,
	const Vector3& eyePos,
	float nearClip,
	float farClip
)
{
	viewMatrix_ = view;
	projectionMatrix_ = projection;
	eyePos_ = eyePos;

	// GlobalConstantsを更新
	globalConstants_->Update(view, projection, eyePos, nearClip, farClip, lightManager_->GetDirectionalLightData()[0]);

	// Rendererにセット（描画パス用）
	rendererManager_->SetCameraState(view, projection, eyePos);
}

void Engine::BeginFrame()
{
	// ImGuiのフレーム開始
	ImGuiManager::BeginFrame();

	// デバッグカメラ更新
	debugCamera_->Update();

	// 時間の更新
	TimeManager::GetInstance()->Update();

	// ポストエフェクトのパラメータ更新など
	GetPostEffectManager()->Update();

#ifdef IS_DEVELOPMENT
	uint32_t finalSrvIndex = GetPostEffectManager()->GetFinalPassSRVIndex();
	debugGuiManager_->BeginSceneView(srvManager_.get(), finalSrvIndex);
#endif
	rendererManager_->BeginFrame();
}

void Engine::EndFrame()
{
	// パイプラインに描画を丸投げ
	RenderCameraState camState = { viewMatrix_, projectionMatrix_, eyePos_ };
	renderPipeline_->Render(this, rendererManager_.get(), commandManager_.get(), camState);

	// フレーム待機（システム処理）
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理（
	uint64_t completedFenceValue = renderPipeline_->GetRenderCoordinator()->GetFenceValue();
	for (auto& textureResource : textureLoader_->GetNewUploads())
	{
		textureLoader_->RegisterPendingUpload(textureResource.intermediate, completedFenceValue);
	}
	textureLoader_->ClearNewUploads();
	textureLoader_->CleanupCompletedUploads(fence_->GetCompletedValue()); // ※適宜Fence修正
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

	// DXGIファクトリの生成
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	// GPUアダプタの選択とD3D12デバイスの作成
	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	graphicsDevice_->Initialize();

	// コマンド関連の初期化
	commandManager_ = std::make_unique<CommandManager>();
	commandManager_->Initialize(graphicsDevice_->GetDevice());

	// スワップチェーンの初期化（画面表示用のバッファ管理）
	swapChain_ = std::make_unique<SwapChain>();
	swapChain_->Initialize(window_->GetHwnd(), commandManager_->GetCommandQueue(),
		GetClientWidth(), GetClientHeight(), 2, dxgiFactory_);

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
	offscreenRTVManager_->Initialize(graphicsDevice_->GetDevice(), srvManager_.get(), descriptorManager_.get(), 20);

	// ライトマネージャの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(graphicsDevice_->GetDevice());

	// カメラマネージャの初期化
	globalConstants_ = std::make_unique<GlobalConstants>();
	globalConstants_->Initialize(graphicsDevice_->GetDevice());
}

void Engine::InitializeRenderer()
{
	// メイン深度ステンシル
	D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle =
		dsvManager_->CreateDepthStencilView(
			GetClientWidth(),
			GetClientHeight(),
			depthStencilResource_
		);

	// オフスクリーン深度ステンシル
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle =
		dsvManager_->CreateDepthStencilView(
			GetClientWidth(),
			GetClientHeight(),
			offscreenDepthResource_
		);

	// フェンス作成（GPU同期用）
	HRESULT hr = graphicsDevice_->GetDevice()->CreateFence(
		fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)
	);
	assert(SUCCEEDED(hr));

	fenceEvent_ = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent_ != nullptr);

	// レンダリング制御クラス初期化
	renderContext_ = std::make_unique<RenderContext>(GetClientWidth(), GetClientHeight());

	// DXC 初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));

	// シェーダ管理
	shaderManager_ = std::make_unique<ShaderManager>();
	shaderManager_->Initialize(
		dxcUtils_.Get(),
		dxcCompiler_.Get(),
		includeHandler_.Get()
	);

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
	renderPipeline_->Initialize(this, mainDsvHandle, offscreenDsvHandle);
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
		GetClientWidth(),
		GetClientHeight(),
		GetShadowMap()
	);

	// 共通ハンドル初期化
	TextureManager::GetInstance().LoadAllTextures(this);
	ModelManager::GetInstance().LoadFromCSV();
	AnimationManager::GetInstance()->LoadFromCSV();
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

#ifdef IS_DEVELOPMENT
	DebugDraw::Initialize(rendererManager_.get());
#endif
}

void Engine::InitializeAudio()
{
	// XAudioエンジン
	AudioDevice::GetInstance().Initialize();
	AudioManager::Initialize();
}
