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
#include "DebugDraw.h"

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

	// srvManager_を使うクラスを先に解放
	textureManager_.reset();     
	postEffectManager_.reset();  

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

	globalConstants_->Update(*camera_);

#ifdef _DEBUG
	debugGuiManager_->Update();

	uint32_t finalSrvIndex = postEffectManager_->GetFinalPassSRVIndex();
	debugGuiManager_->BeginSceneView(srvManager_.get(), finalSrvIndex);
#endif
	renderer_->BeginFrame();
}

void Engine::EndFrame()
{
	auto* cmdList = commandManager_->GetCommandList();

	// シャドウパス
	shadowMap_->TransitionToDepthWrite(cmdList);
	D3D12_CPU_DESCRIPTOR_HANDLE shadowDSV = shadowMap_->GetDSVHandle();
	cmdList->OMSetRenderTargets(0, nullptr, FALSE, &shadowDSV);
	cmdList->ClearDepthStencilView(
		shadowDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
	);

	D3D12_VIEWPORT shadowVP = { 0.0f, 0.0f, 2048.0f, 2048.0f, 0.0f, 1.0f };
	D3D12_RECT shadowRect = { 0, 0, 2048, 2048 };
	cmdList->RSSetViewports(1, &shadowVP);
	cmdList->RSSetScissorRects(1, &shadowRect);

	renderer_->DrawSceneForShadow();
	shadowMap_->TransitionToRead(cmdList);

	// メインパス（オフスクリーン描画）
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenRTV = renderCoordinator_->GetOffscreenRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenDSV = renderCoordinator_->GetOffscreenDSVHandle();
	cmdList->OMSetRenderTargets(1, &offscreenRTV, FALSE, &offscreenDSV);
	cmdList->ClearDepthStencilView(
		offscreenDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
	);
	cmdList->RSSetViewports(1, &renderContext_->GetViewport());
	cmdList->RSSetScissorRects(1, &renderContext_->GetScissorRect());

	renderer_->Draw3D();
	renderCoordinator_->EndOffscreenRender();

	// ポストエフェクト（Bloomなど）
	postEffectManager_->ExecutePostEffects(cmdList);

	// バックバッファ準備（直後に描画先は切り替える）
	renderCoordinator_->BeginFrame();

	// FinalBuffer（SRV → RenderTarget）
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			postEffectManager_->GetFinalPassResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		cmdList->ResourceBarrier(1, &barrier);
	}

	// 最終合成・トーンマップ
	D3D12_CPU_DESCRIPTOR_HANDLE finalRTV = postEffectManager_->GetFinalPassRTV();
	cmdList->OMSetRenderTargets(1, &finalRTV, FALSE, nullptr);

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	renderer_->DrawFullScreenQuadWithOffscreenTexture();
#ifdef _DEBUG
	renderer_->DrawUI();
#endif

	// FinalBuffer：RenderTarget → SRV
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			postEffectManager_->GetFinalPassResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		cmdList->ResourceBarrier(1, &barrier);
	}

	// バックバッファ出力
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV =
		rtvManager_->GetCurrentBackBufferRTVCPUHandle(swapChain_.get());
	cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

#ifdef _DEBUG
	// シーンウィンドウを閉じる
	debugGuiManager_->EndSceneView();
#else
	cmdList->RSSetViewports(1, &renderContext_->GetViewport());
	cmdList->RSSetScissorRects(1, &renderContext_->GetScissorRect());
	// 最終結果をバックバッファへ描画
	uint32_t finalSrvIndex = postEffectManager_->GetFinalPassSRVIndex();
	renderer_->DrawFinalResult(finalSrvIndex);
	renderer_->DrawUI();
#endif

	// ImGui描画
	cmdList->SetDescriptorHeaps(1, heaps);
	ImGuiManager::EndFrame(cmdList);

	// フレーム終了処理
	renderCoordinator_->EndFrame();
	frameLimiter_->WaitNextFrame();

	// アップロードリソース管理
	uint64_t completedFenceValue = renderCoordinator_->GetFenceValue();
	for (auto& textureResource : textureManager_->GetNewUploads())
	{
		textureManager_->RegisterPendingUpload(
			textureResource.intermediate, completedFenceValue
		);
	}
	textureManager_->ClearNewUploads();
	textureManager_->CleanupCompletedUploads(
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
	dsvManager_->Initialize(graphicsDevice_->GetDevice(), descriptorManager_.get(), srvManager_.get(), 8); 

	// RTVマネージャの初期化
	rtvManager_ = std::make_unique<RTVManager>();
	rtvManager_->Initialize(graphicsDevice_->GetDevice(), swapChain_->GetSwapChain(),
		2, descriptorManager_.get());

	// オフスクリーンレンダーターゲットの初期化
	offscreenRTVManager_ = std::make_unique<OffscreenRTVManager>();
	offscreenRTVManager_->Initialize(graphicsDevice_->GetDevice(), srvManager_.get(), descriptorManager_.get(), 16);

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
			kClientWidth,
			kClientHeight,
			depthStencilResource_
		);

	// オフスクリーンレンダーターゲット
	auto [offscreenTexture, offscreenRtvHandle] =
		offscreenRTVManager_->CreateOffscreenRenderTarget(
			kClientWidth,
			kClientHeight,
			offscreenRTVManager_->GetClearColor()
		);

	// オフスクリーン深度ステンシル
	D3D12_CPU_DESCRIPTOR_HANDLE offscreenDsvHandle =
		dsvManager_->CreateDepthStencilView(
			kClientWidth,
			kClientHeight,
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
		mainDsvHandle,
		offscreenRtvHandle,
		offscreenTexture.Get(),
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
		kClientWidth,
		kClientHeight,
		rootSignatureManager_.get(),
		psoManager_.get(),
		camera_,
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
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(
		graphicsDevice_->GetDevice(),
		commandManager_->GetCommandList(),
		srvManager_.get()
	);

	// レンダラー
	renderer_ = std::make_unique<Renderer>();
	renderer_->Initialize(
		graphicsDevice_.get(),
		commandManager_.get(),
		psoManager_.get(),
		rootSignatureManager_.get(),
		textureManager_.get(),
		srvManager_.get(),
		lightManager_.get(),
		globalConstants_.get(),
		materialManager_,
		camera_,
		postEffectManager_.get(),
		kClientWidth,
		kClientHeight,
		shadowMap_.get()
	);

	// 共通ハンドル初期化
	TextureHandle::Initialize(this);
	ParticleTextureHandle::Initialize(this);
	ModelHandle::Initialize(this);
	AnimationHandle::Initialize();

	// テクスチャ配列
	std::vector<std::string> texturePaths = {
		"Assets/Textures/uvChecker.png",
	};

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
