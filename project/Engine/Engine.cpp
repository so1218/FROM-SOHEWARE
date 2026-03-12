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
	debugGuiManager_->Initialize(this, lightManager_.get(), materialManager_.get(), textureLoader_.get(), postEffectManager_.get(), debugCamera_.get());
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
	postEffectManager_.reset();  

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
	postEffectManager_->Update();

#ifdef IS_DEVELOPMENT
	uint32_t finalSrvIndex = postEffectManager_->GetFinalPassSRVIndex();
	debugGuiManager_->BeginSceneView(srvManager_.get(), finalSrvIndex);
#endif
	rendererManager_->BeginFrame();
}

void Engine::EndFrame()
{
	auto* cmdList = commandManager_->GetCommandList();

	// シャドウパス
	shadowMap_->BeginPass(cmdList);
	rendererManager_->DrawSceneForShadow();
	shadowMap_->EndPass(cmdList);

	// G-Buffer / オフスクリーンパス
	renderCoordinator_->BeginOffscreenRender();
	rendererManager_->Draw3D();
	renderCoordinator_->EndOffscreenRender();

	// ポストエフェクトパス
	postEffectManager_->ExecutePostEffects(cmdList, viewMatrix_, projectionMatrix_, eyePos_);

	// 最終合成・トーンマップパス
	renderCoordinator_->BeginFrame(); // (バックバッファの準備など)

	postEffectManager_->BeginFinalComposite(cmdList);
	rendererManager_->DrawFullScreenQuadWithOffscreenTexture();
#ifdef IS_DEVELOPMENT
	rendererManager_->DrawUI();
#endif
	postEffectManager_->EndFinalComposite(cmdList);

	// バックバッファへの転送
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = rtvManager_->GetCurrentBackBufferRTVCPUHandle(swapChain_.get());
	cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

#ifdef IS_DEVELOPMENT
	debugGuiManager_->EndSceneView();
#else
	cmdList->RSSetViewports(1, &renderContext_->GetViewport());
	cmdList->RSSetScissorRects(1, &renderContext_->GetScissorRect());
	rendererManager_->DrawFinalResult(postEffectManager_->GetFinalPassSRVIndex());
	rendererManager_->DrawUI();
#endif

	// UIとフレーム終了処理
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	ImGuiManager::EndFrame(cmdList);

	renderCoordinator_->EndFrame();
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理
	uint64_t completedFenceValue = renderCoordinator_->GetFenceValue();
	for (auto& textureResource : textureLoader_->GetNewUploads())
	{
		textureLoader_->RegisterPendingUpload(
			textureResource.intermediate, completedFenceValue
		);
	}
	textureLoader_->ClearNewUploads();
	textureLoader_->CleanupCompletedUploads(
		renderCoordinator_->GetFence()->GetCompletedValue()
	);
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
	renderCoordinator_ = std::make_unique<RenderCoordinator>();
	renderCoordinator_->Initialize(
		swapChain_.get(),
		rtvManager_.get(),
		offscreenRTVManager_.get(),
		commandManager_.get(),
		renderContext_.get(),
		fence_.Get(),
		fenceEvent_,
		graphicsDevice_.get(),
		this,
		mainDsvHandle,
		offscreenDsvHandle,
		offscreenDepthResource_.Get()
	);

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

	// オフスクリーン深度の SRV インデックス
	uint32_t offscreenDepthSrvIndex =
		dsvManager_->GetDSVTextureSRVIndex(1);

	// ポストエフェクト
	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(
		this,
		GetClientWidth(),
		GetClientHeight(),
		rootSignatureManager_.get(),
		psoManager_.get(),
		srvManager_.get(),
		offscreenDepthSrvIndex
	);

	postEffectManager_->SetSceneDepthIndex(offscreenDepthSrvIndex);

	// シャドウマップ
	shadowMap_ = std::make_unique<ShadowMap>();
	shadowMap_->Initialize(
		graphicsDevice_->GetDevice(),
		2048,
		2048,
		srvManager_.get()
	);
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
		postEffectManager_.get(),
		GetClientWidth(),
		GetClientHeight(),
		shadowMap_.get()
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

int Engine::LoadTexture(const std::string& texturePath)
{
	return rendererManager_->LoadTexture(texturePath);
}
