#include "Engine.h"
#include "AudioManager.h"
#include "DebugLayerManager.h"
#include "Logger.h"
#include "Input.h"
#include "ImGuiManager.h"
#include "ShapeGenerator.h"
#include "DSVManager.h"
#include "TimeManager.h"
#include "ModelHandle.h"
#include "TextureHandle.h"
#include "ParticleTextureHandle.h"
#include "AudioHandle.h"
#include "AnimationHandle.h"
#include "GlobalVariables.h"

#include "externals/DirectXTex/d3dx12.h" 

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "winmm.lib")

std::wstring Engine::windowTitle_ = L"FROM SOHEWARE";
int Engine::kFixedFPS_ = 60;

void Engine::Initialize(Camera* camera, MaterialManager* materialManager)
{
	materialManager_ = materialManager;
	camera_ = camera;
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
	debugGuiManager_->Initialize(this, camera_, lightManager_.get(), materialManager_, textureManager_.get(), postEffectManager_.get(), debugCamera_.get());
	particleSystem_ = std::make_unique<ParticleSystem>();
	particleSystem_->Initialize(this);
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

	// InitializeRendererで確保した深度SRVを解放
	if (srvManager_ && postEffectManager_) 
	{
		srvManager_->FreeSRV(postEffectManager_->sceneDepthIndex_);
	}

	// srvManager_ を使うクラスを先に解放する
	textureManager_.reset();     
	postEffectManager_.reset();  

	// 依存されていた srvManager_ を解放する
	srvManager_.reset();

	// リソース解放
	CloseHandle(fenceEvent_);
	CloseWindow(window_->GetHwnd());

	CoUninitialize();
}

void Engine::BeginFrame()
{
	// ImGuiなどUIのフレーム開始
	ImGuiManager::BeginFrame();

	// デバッグカメラ更新
	debugCamera_->Update();

	// 時間の更新
	TimeManager::GetInstance()->Update();

	// オフスクリーンレンダリングの準備開始
	renderCoordinator_->BeginOffscreenRender();

	// ポストエフェクトのパラメータ更新など
	postEffectManager_->Update();

	cameraManager_->GetFrameData()->cameraWorldPosition = camera_->GetTranslation();

#ifdef _DEBUG
	debugGuiManager_->Update();
#endif
	renderer_->BeginFrame();
}

void Engine::EndFrame()
{
	// オフスクリーンレンダリング終了
	renderCoordinator_->EndOffscreenRender();

	// フレームレンダリング開始
	renderCoordinator_->BeginFrame();

	// ポストエフェクト適用
	postEffectManager_->ExecutePostEffects(commandManager_->GetCommandList());

	// バックバッファのRTVをセット
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCurrentBackBufferRTVCPUHandle(swapChain_.get());
	commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	
	// ImGui用DescriptorHeapをCommandListにバインド
	ID3D12DescriptorHeap* defaultHeaps[] = { srvManager_->GetSRVHeap()};
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(defaultHeaps), defaultHeaps);
	
#ifdef _DEBUG
	if (useDebugView_)
	{
		debugGuiManager_->RenderOffscreenTexture(
			srvManager_.get(),
			postEffectManager_->bloomCombineIndex_
		);
	}
	else
	{
		renderer_->DrawFullScreenQuadWithOffscreenTexture();
	}

#else
	renderer_->DrawFullScreenQuadWithOffscreenTexture();
#endif

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// ImGui 描画コマンド積む
	ImGuiManager::EndFrame(commandManager_->GetCommandList());

	// フレームレンダリング終了
	renderCoordinator_->EndFrame();

	// FPS固定
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理
	uint64_t completedFenceValue = renderCoordinator_->GetFenceValue();
	for (auto& textureResource : textureManager_->GetNewUploads())
	{
		textureManager_->RegisterPendingUpload(textureResource.intermediate, completedFenceValue);
	}
	textureManager_->ClearNewUploads();
	textureManager_->CleanupCompletedUploads(renderCoordinator_->GetFence()->GetCompletedValue());
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
	window_ = std::make_unique<Window>(kClientWidth, kClientHeight);
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
		kClientWidth, kClientHeight, 2, dxgiFactory_);

	// ディスクリプタヒープマネージャの作成
	descriptorManager_ = std::make_unique<DescriptorHeapManager>();

	// SRVマネージャの初期化
	srvManager_ = std::make_unique<SRVManager>();
	srvManager_->Initialize(graphicsDevice_->GetDevice(), 1000);

	// DSVマネージャの初期化
	dsvManager_ = std::make_unique<DSVManager>();
	dsvManager_->Initialize(graphicsDevice_->GetDevice(), descriptorManager_.get(), 8); 

	// RTVマネージャの初期化
	rtvManager_ = std::make_unique<RTVManager>();
	rtvManager_->Initialize(graphicsDevice_->GetDevice(), swapChain_->GetSwapChain(),
		2, descriptorManager_.get());

	// オフスクリーンレンダーターゲットの初期化
	offscreenRTVManager_ = std::make_unique<OffscreenRTVManager>();
	offscreenRTVManager_->Initialize(graphicsDevice_->GetDevice(), srvManager_.get(), descriptorManager_.get(), 16);

	// ライト（光源）マネージャの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(graphicsDevice_->GetDevice());

	// カメラマネージャの初期化
	cameraManager_ = std::make_unique<CameraManager>();
	cameraManager_->Initialize(graphicsDevice_->GetDevice());
}

void Engine::InitializeRenderer()
{
	// 深度ステンシルバッファと対応するDSVの作成
	D3D12_CPU_DESCRIPTOR_HANDLE mainDsvHandle = dsvManager_->CreateDepthStencilView(
		kClientWidth,
		kClientHeight,
		depthStencilResource_ // 深度ステンシル用リソースを生成・取得
	);

	// オフスクリーンレンダーターゲットの作成
	auto [offscreenTexture, offscreenRtvHandle] =
		offscreenRTVManager_->CreateOffscreenRenderTarget(
			kClientWidth, kClientHeight, offscreenRTVManager_->GetClearColor()
		);

	// オフスクリーン用の深度ステンシルバッファを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> offscreenDepthResource;
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle = dsvManager_->CreateDepthStencilView(
		kClientWidth, kClientHeight, offscreenDepthResource
	);

	// フェンスとイベントの作成（GPUの処理完了を待機するため）
	HRESULT hr = graphicsDevice_->GetDevice()->CreateFence(
		fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// フェンスシグナルを待機するためのイベントを作成
	fenceEvent_ = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent_ != nullptr);

	// レンダリングコンテキストおよびレンダーコーディネーターの初期化
	renderContext_ = std::make_unique<RenderContext>(kClientWidth, kClientHeight);
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
		// 各種レンダーターゲット・DSVハンドルを渡す
		mainDsvHandle,
		offscreenRtvHandle,
		offscreenTexture.Get(), // バリア処理用にリソースを渡す
		offscreenDsvHandle
	);

	// DXCコンパイラ関連の初期化
	HRESULT hr1 = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr1));
	HRESULT hr2 = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr2));

	// インクルードパス対応のためのハンドラを作成
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));

	// ShaderManager を生成・初期化
	shaderManager_ = std::make_unique<ShaderManager>();
	shaderManager_->Initialize(dxcUtils_.Get(), dxcCompiler_.Get(), includeHandler_.Get());

	// ルートシグネチャの初期化
	rootSignatureManager_ = std::make_unique<RootSignatureManager>();
	rootSignatureManager_->Initialize(graphicsDevice_->GetDevice());

	// PSOの初期化
	psoManager_ = std::make_unique<PSOManager>();
	psoManager_->Initialize(
		graphicsDevice_->GetDevice(),
		shaderManager_.get(),
		rootSignatureManager_.get()
	);

	// ポストエフェクト管理の初期化
	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(
		this,
		graphicsDevice_->GetDevice(),
		offscreenRTVManager_.get(),
		kClientWidth, kClientHeight,
		rootSignatureManager_.get(),
		psoManager_.get(),
		camera_,
		srvManager_.get()
	);

	// 深度バッファ用のSRVを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
	depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrvDesc.Texture2D.MipLevels = 1;

	// 深度バッファをシェーダから参照するためのSRVを作成
	postEffectManager_->sceneDepthIndex_ = srvManager_->CreateSRV(
		depthStencilResource_.Get(),
		depthSrvDesc
	);
}

void Engine::InitializeResources()
{
	// テクスチャマネージャの初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(graphicsDevice_->GetDevice(), commandManager_->GetCommandList(), srvManager_.get());

	// レンダラーの初期化
	renderer_ = std::make_unique<Renderer>();
	renderer_->Initialize(
		graphicsDevice_.get(),
		commandManager_.get(),
		psoManager_.get(),
		rootSignatureManager_.get(),
		textureManager_.get(),
		srvManager_.get(),     
		lightManager_.get(),
		cameraManager_.get(),
		materialManager_,      
		camera_,   
		postEffectManager_.get(),
		kClientWidth,
		kClientHeight
	);

	// 各種ハンドルクラスの初期化（エンジン全体で共通的に利用）
	TextureHandle::Initialize(this);
	ParticleTextureHandle::Initialize(this);
	ModelHandle::Initialize(this);
	AnimationHandle::Initialize();

	// 配列テクスチャのパスを用意
	std::vector<std::string> texturePaths = {
		"Resources/images/uvChecker.png",

	};

	// テクスチャ配列を読み込み、GPUにアップロード
	LoadTextureArray(texturePaths);
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
}

void Engine::InitializeAudio()
{
	// XAudioエンジン
	AudioManager::GetInstance().Initialize();
	AudioHandle::Initialize();
}

int Engine::LoadTexture(const std::string& texturePath)
{
	return renderer_->LoadTexture(texturePath);
}

void Engine::LoadTextureArray(const std::vector<std::string>& texturePaths)
{
	renderer_->LoadTextureArray(texturePaths);
}
