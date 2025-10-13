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
#include "AudioHandle.h"
#include "AnimationHandle.h"

#include "externals/DirectXTex/d3dx12.h" 

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

std::wstring Engine::windowTitle_ = L"FROM SOHEWARE";	

const int32_t Engine::kMaxTriangleCount = 0; // 三角形の最大数
const int32_t Engine::kMaxSphereCount = 0; // 球の最大数
const int32_t Engine::kMaxModelCount = 500; // モデルの最大数
const int32_t Engine::kMaxSpriteCount = 101; // スプライトの最大数
const int32_t Engine::kMaxCubeCount = 0;// 立方体の最大数
const int32_t Engine::kMaxLineCount = 400;// ラインの最大数
const int32_t Engine::kMaxParticleCount = 1000;// パーティクルの最大数

void Engine::Initialize(Camera* camera, MaterialManager* materialManager)
{
	materialManager_ = materialManager;
	camera_ = camera;
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize();

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

	CreateObjects();
}

// 毎フレーム使う描画インデックスをリセット
void Engine::ResetDrawCounters()
{
	// 描画カウンタの初期化(毎フレーム)
	indexSphere_ = 0;
	indexModel_ = 0;
	indexSprite_ = 0;
	indexTriangle_ = 0;
	indexCube_ = 0;
	indexLine_ = 0;
	indexParticle_ = 0;
	indexInstance_ = 0;
};

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
}

void Engine::EndFrame()
{
	Logger::Instance().Log("  UsedCount: " + std::to_string(srvAllocator_->GetUsedCount()));
	Logger::Instance().Log("  FreeCount: " + std::to_string(srvAllocator_->GetFreeCount()));
	Logger::Instance().Log("  MaxDescriptors: " + std::to_string(srvAllocator_->GetMaxDescriptors()));

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
	ID3D12DescriptorHeap* defaultHeaps[] = { srvDescriptorHeap_.Get()};
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(defaultHeaps), defaultHeaps);
	
#ifdef _DEBUG
	if (useDebugView)
	{
		debugGuiManager_->RenderOffscreenTexture(
			srvDescriptorHeap_.Get(),
			descriptorSizeSRV_,
			offscreenRTVManager_->GetSRVHandleCPU(postEffectManager_->bloomCombineIndex_),
			offscreenSrvIndex_
		);
	}
	else
	{
		DrawFullScreenQuadWithOffscreenTexture();
	}

#else
	DrawFullScreenQuadWithOffscreenTexture();
#endif

	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// ImGui 描画コマンド積む
	ImGuiManager::EndFrame(commandManager_->GetCommandList());

	// フレームレンダリング終了
	renderCoordinator_->EndFrame();

	// アップロードリソース管理
	uint64_t completedFenceValue = renderCoordinator_->GetFenceValue();
	for (auto& textureResource : textureManager_->GetNewUploads())
	{
		textureManager_->RegisterPendingUpload(textureResource.intermediate, completedFenceValue);
	}
	textureManager_->ClearNewUploads();
	textureManager_->CleanupCompletedUploads(renderCoordinator_->GetFence()->GetCompletedValue());
}

void Engine::DrawFullScreenQuadWithOffscreenTexture()
{
	// コマンドリストのローカル変数を取得
	auto* cmdList = commandManager_->GetCommandList();

	// SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { offscreenRTVManager_->GetSRVDescriptorHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	// パイプラインステートをセット（フルスクリーン描画用PSO）
	cmdList->SetPipelineState(psoManager_->psoFullscreen_.Get());

	// ルートシグネチャをセット（フルスクリーン用のルートシグネチャ）
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureFullScreen_.Get());

	// ルートパラメータにSRVなどをセット
	cmdList->SetGraphicsRootDescriptorTable(1, offscreenRTVManager_->GetSRVHandleGPU(postEffectManager_->bloomCombineIndex_));
	cmdList->SetGraphicsRootConstantBufferView(0, postEffectManager_->constantBuffer_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, offscreenRTVManager_->GetSRVHandleGPU(postEffectManager_->depthExtractIndex_));

	// プリミティブトポロジーを設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファなし

	// DrawCall発行（3頂点の三角形）
	cmdList->DrawInstanced(3, 1, 0, 0);
}

void Engine::PreDraw()
{
	cameraManager_->GetCameraData()->worldPosition = camera_->GetTranslation();

#ifdef _DEBUG
	debugGuiManager_->Update();
#endif

	ResetDrawCounters();
}

void Engine::InitializeSystem()
{
	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// 誰も捕捉しなかった場合に、補足する関数を登録
	SetUnhandledExceptionFilter(Logger::ExportDump);

	// ロガーの初期化
	Logger::Instance().Initialize();
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
	Input::Initialize(window_->GetHInstance(), window_->GetHwnd());

	TimeManager::GetInstance()->Initialize();
}

void Engine::InitializeGraphics()
{
	// デバッグレイヤーの初期化
#ifdef _DEBUG
	DebugLayerManager::Instance().Initialize();
#endif

	// DXGIファクトリーの生成
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	// GPUアダプタの選定+D3D12デバイスの生成
	graphicDevice_ = std::make_unique<GraphicDevice>();
	graphicDevice_->Initialize();

	Logger::Instance().Log("Complete create D3D12Device!!!\n", Logger::Instance().GetLogStream());// 初期化完了のログを出す

	// コマンドの初期化
	commandManager_ = std::make_unique<CommandManager>();
	commandManager_->Initialize(graphicDevice_->GetDevice());

	// スワップチェーンの初期化
	swapChain_ = std::make_unique<SwapChain>();
	swapChain_->Initialize(window_->GetHwnd(), commandManager_->GetCommandQueue(), kClientWidth, kClientHeight, 2, dxgiFactory_);

	// SRVAllocatorの初期化（最大数128と仮定、必要に応じて調整）
	srvAllocator_ = std::make_unique<SRVAllocator>(128);

	// DescriptorSizeを取得しておく
	descriptorSizeSRV_ = graphicDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV_ = graphicDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = graphicDevice_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// レンダーターゲットの初期化
	rtvManager_ = std::make_unique<RTVManager>();
	rtvManager_->Initialize(graphicDevice_->GetDevice(), swapChain_->GetSwapChain(), 2, descriptorSizeRTV_, descriptorManager_.get());
	offscreenRTVManager_ = std::make_unique<OffscreenRTVManager>();
	offscreenRTVManager_->Initialize(graphicDevice_->GetDevice(), descriptorManager_.get(), 16);
	offscreenRTVManager_->CreateOffscreenRenderTarget(kClientWidth, kClientHeight, offscreenRTVManager_->GetClearColor());

	// ディスクリプタヒープの作成
	descriptorManager_ = std::make_unique<DescriptorManager>();
	// SRV用のヒープでディスクリプタの数は1000。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap_ = descriptorManager_->CreateDescriptorHeap(graphicDevice_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000, true);
	// DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	dsvDescriptorHeap_ = descriptorManager_->CreateDescriptorHeap(graphicDevice_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// 光源の初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(graphicDevice_->GetDevice());

	cameraManager_ = std::make_unique<CameraManager>();
	cameraManager_->Initialize(graphicDevice_->GetDevice());
}

void Engine::InitializeRenderer()
{
	// DepthStencilTextureをウィンドウのサイズで作成
	depthStencilResource_ = DSVManager::CreateDepthStencilTextureResource(graphicDevice_->GetDevice(), kClientWidth, kClientHeight);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;// Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;// 2dTexture
	// DSVHeapの先頭にDSVをつくる
	graphicDevice_->GetDevice()->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

	// フェンスとイベント
	HRESULT hr = graphicDevice_->GetDevice()->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));
	// FenceのSignalを待つためのイベントを作成する
	fenceEvent_ = CreateEvent(nullptr, false, false, nullptr);
	assert(fenceEvent_ != nullptr);

	// レンダラーの初期化
	renderContext_ = std::make_unique<RenderContext>(kClientWidth, kClientHeight);
	renderCoordinator_ = std::make_unique<RenderCoordinator>();
	renderCoordinator_->Initialize(swapChain_.get(), rtvManager_.get(), offscreenRTVManager_.get(), commandManager_.get(),
	renderContext_.get(), fence_.Get(), fenceEvent_, graphicDevice_.get(), dsvDescriptorHeap_.Get(), this);
	
	// dxcCompilerを初期化
	HRESULT hr1 = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr1));
	HRESULT hr2 = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr2));

	// 現時点でincludeしないが、includeに対応するための設定を行っておく
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));

	// ルートシグネチャの作成
	rootSignatureManager_ = std::make_unique<RootSignatureManager>();
	rootSignatureManager_->Initialize(graphicDevice_->GetDevice());

	// PSOの作成
	psoManager_ = std::make_unique<PSOManager>();
	psoManager_->Initialize(graphicDevice_->GetDevice(), dxcUtils_.Get(), dxcCompiler_.Get(), includeHandler_.Get(), rootSignatureManager_.get());

	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(this, graphicDevice_->GetDevice(), offscreenRTVManager_.get(), kClientWidth, kClientHeight, rootSignatureManager_.get(), psoManager_.get(), camera_);

	// DepthのSRV用Indexを確保
	postEffectManager_->sceneDepthIndex_ = srvAllocator_->Allocate();

	// CPUハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE depthSRV_CPU = srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	depthSRV_CPU.ptr += postEffectManager_->sceneDepthIndex_ * descriptorSizeSRV_;

	// SRVの記述
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	// SRVを作成
	graphicDevice_->GetDevice()->CreateShaderResourceView(
		depthStencilResource_.Get(), &srvDesc, depthSRV_CPU);
}

void Engine::InitializeResources()
{
	offscreenSrvIndex_ = srvAllocator_->Allocate();

	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(graphicDevice_->GetDevice(), commandManager_->GetCommandList(), srvAllocator_.get());
	TextureHandle::Initialize(this);
	ModelHandle::Initialize(this);
	AnimationHandle::Initialize();

	// 配列テクスチャのパスを用意
	std::vector<std::string> texturePaths = {
		"Resources/images/uvChecker.png",

	};

	// Texture2DArray作成＆アップロード
	LoadTextureArray(texturePaths);
}

void Engine::InitializeImGui()
{
	// ImGuiの初期化
	ImGuiManager::Initialize(window_->GetHwnd(), graphicDevice_->GetDevice(),
		rtvManager_->rtvDesc, swapChain_->GetSwapChainDesc(), srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
}

void Engine::InitializeAudio()
{
	// XAudioエンジン
	AudioManager::GetInstance().Initialize();
	AudioHandle::Initialize();
}

int Engine::LoadTexture(const std::string& texturePath)
{
	// テクスチャをロード
	DirectX::ScratchImage mipImages = TextureManager::LoadTexture(texturePath);

	// テクスチャをアップロード
	TextureManager::TextureResources texResources = textureManager_->UploadTexture(mipImages, srvDescriptorHeap_.Get(), *graphicDevice_, descriptorSizeSRV_, textures_);

	// 保存したテクスチャのインデックスを返す
	return static_cast<int>(textures_.size()) - 1;
}

void Engine::LoadTextureArray(const std::vector<std::string>& texturePaths)
{
	// 1. 複数テクスチャをロード
	std::vector<DirectX::ScratchImage> images = textureManager_->LoadMultipleTextures(texturePaths);

	// 2. Texture2DArray作成＆アップロード
	textureManager_->CreateAndUploadTexture2DArray(images, srvDescriptorHeap_.Get(), descriptorSizeSRV_, textureArrayResource_);

	// 3. textureArraySRV_を内部で管理
	textureManager_->textureArraySRV_ = textureArrayResource_.srvHandleGPU;
}

Matrix4x4 MakeCenteredAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate, Vector3 pivot)
{
	Matrix4x4 moveToOrigin = Matrix4x4::MakeTranslate({ -pivot.x, -pivot.y, -pivot.z });
	Matrix4x4 rotateScale = Matrix4x4::MakeAffine(scale, rotate, { 0.0f, 0.0f, 0.0f });
	Matrix4x4 moveBack = Matrix4x4::MakeTranslate(pivot);
	Matrix4x4 result = (moveToOrigin * rotateScale) * moveBack;
	return result * Matrix4x4::MakeTranslate(translate);
}

void Engine::CreateObjects()
{
	CreateSpheres();
	CreateModels();
	CreateSprites();
	CreateTriangles();
	CreateCubes();
	CreateLines();
	CreateParticles();
}

void Engine::CreateTriangles()
{
	// 最大数の三角形分の配列を確保
	triangles_.resize(kMaxTriangleCount);

	// 初期仮の頂点データ(これは後で上書きされる)
	std::vector<VertexData> triangleVertices =
	{
		{{ 0.0f,   360.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, {0.0f,0.0f,1.0f}},
		{{ 320.0f,   0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f }, {0.0f,0.0f,1.0f}},
		{{ 640.0f, 360.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, {0.0f,0.0f,1.0f}}
	};

	// 指定数分の三角形メッシュとリソースを初期化
	for (size_t i = 0; i < kMaxTriangleCount; ++i)
	{
		// メッシュ初期化(vertex + index データをGPUへ転送)
		triangles_[i].mesh.InitializeVertexOnly(graphicDevice_->GetDevice(), triangleVertices);
		// マテリアルを作成・設定
		triangles_[i].materialHandle = materialManager_->CreateMaterial(graphicDevice_->GetDevice());

		triangles_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();// UV行列は単位行列で初期化

		// WVP行列用のバッファを作成
		triangles_[i].wvpResource = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		triangles_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&triangles_[i].mappedData));  // CPUアクセス用にマッピング

		triangles_[i].mesh.SetVertexCount(triangleVertices.size()); // インデックス数を設定
	}

	// 最初に使用するスフィアのインデックスをリセット
	indexTriangle_ = 0;
}
void Engine::DrawTriangle(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	// indexTriangle_が範囲内であることを確認
	assert(indexTriangle_ < kMaxTriangleCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する三角形を取得
	RenderData& triangle = triangles_[indexTriangle_];

	// マテリアルに色情報を設定
	triangle.materialHandle.materialData->color = Uint32ToColorVector(color);

	// ワールド行列（スケール・回転・移動）を計算
	Vector3 pivot = { 320.0f, 180.0f, 0.0f }; // 三角形の中心
	triangle.worldMatrix = MakeCenteredAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_, pivot);

	// WVP行列（World * ViewProjection）を計算
	Matrix4x4 wvpMatrix = triangle.worldMatrix * Matrix4x4::MakeOrthographic(0, 0, float(kClientWidth), float(kClientHeight), 0, 100);
	memcpy(&triangle.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	triangle.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(triangle.worldMatrix.Transpose());

	// UV変換行列の設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	triangle.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// プリミティブ形状を設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &triangle.mesh.GetVertexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, triangle.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, triangle.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawInstanced(UINT(triangle.mesh.GetVertexCount()), 1, 0, 0);
	// 使用カウント上昇
	indexTriangle_++;
}

void Engine::CreateSpheres()
{
	spheres_.resize(kMaxSphereCount);

	std::vector<VertexData> sphereVertices;
	std::vector<uint32_t> sphereIndices;

	ShapeGenerator shapeGenerator;
	shapeGenerator.SphereGenerator(sphereVertices, sphereIndices);

	for (size_t i = 0; i < kMaxSphereCount; ++i)
	{
		spheres_[i].mesh.Initialize(graphicDevice_->GetDevice(), sphereVertices, sphereIndices);

		spheres_[i].materialHandle = materialManager_->CreateMaterial(graphicDevice_->GetDevice());
		spheres_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();

		spheres_[i].wvpResource = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		spheres_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&spheres_[i].mappedData));

		spheres_[i].mesh.SetIndexCount(sphereIndices.size());
	}
	indexSphere_ = 0;
}
void Engine::DrawSphere(WorldTransform& worldTransform, Camera& camera, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t color)
{
	// indexSphere_が範囲内であることを確認
	assert(indexSphere_ < kMaxSphereCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する球体を取得
	RenderData& sphere = spheres_[indexSphere_];

	// マテリアルに色情報を設定
	sphere.materialHandle.materialData->color = Uint32ToColorVector(color);

	// ワールド行列を計算
	sphere.worldMatrix = worldTransform.matWorld_;

	// WVP行列（World * ViewProjection）を計算
	Matrix4x4 wvpMatrix = sphere.worldMatrix * camera.GetViewProjectionMatrix();
	memcpy(&sphere.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	sphere.mappedData->World = sphere.worldMatrix;
	sphere.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(sphere.worldMatrix.Transpose());

	// UV変換行列の設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	sphere.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// プリミティブ形状を設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &sphere.mesh.GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&sphere.mesh.GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sphere.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sphere.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sphere.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexSphere_++;
}

Mesh* Engine::GetOrCreateMesh(const ModelData& modelData)
{
	auto it = meshCache.find(&modelData);
	if (it != meshCache.end()) {
		return &it->second;
	}

	Mesh newMesh;
	newMesh.Initialize(graphicDevice_->GetDevice(), modelData.vertices, modelData.indices);
	newMesh.SetVertexCount(static_cast<uint32_t>(modelData.vertices.size()));
	newMesh.SetIndexCount(static_cast<uint32_t>(modelData.indices.size()));

	meshCache[&modelData] = std::move(newMesh);
	return &meshCache[&modelData];
}
void Engine::CreateModels()
{
	models_.resize(kMaxModelCount);

	for (size_t i = 0; i < kMaxModelCount; ++i)
	{
		// マテリアルを作成
		models_[i].materialHandle = materialManager_->CreateMaterial(graphicDevice_->GetDevice());
		models_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();

		// WVP行列用のバッファを作成
		models_[i].wvpResource = BufferManager::CreateBufferResource(
			graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		models_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&models_[i].mappedData));
	}
	indexModel_ = 0;
}

void Engine::UpdateAnimation(AnimatedModelData& instance)
{
	instance.animationTime += TimeManager::GetInstance()->GetDeltaTime();
	instance.animationTime = std::fmod(instance.animationTime, instance.animation.duration);

	NodeAnimation& nodeAnim = instance.animation.nodeAnimations[instance.animation.rootNodeName];
	Vector3 translation = CalculateValue(nodeAnim.translate.keyframes, instance.animationTime);
	Quaternion rotation = CalculateValue(nodeAnim.rotate.keyframes, instance.animationTime);
	Vector3 scale = CalculateValue(nodeAnim.scale.keyframes, instance.animationTime);
	instance.localMatrix = Matrix4x4::MakeAffine(scale, rotation, translation);
}

void Engine::DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color)
{
	// indexModel_が範囲内であることを確認
	assert(indexModel_ < kMaxModelCount);


	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するモデルを取得
	RenderData& model = models_[indexModel_];

	// 一度だけ作られたMeshを使う
	Mesh* mesh = GetOrCreateMesh(modelData);

	// 色変換
	modelData.materialHandle.materialData->color = Uint32ToColorVector(color);

	// ワールド行列 (スケール・回転・位置) を計算
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列 (World * ViewProjection) を計算
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// パイプラインステートの設定
	if (isWireFrame_) {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3DWireframe_.Get());
	}
	else {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	}
	// プリミティブ形状の設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, modelData.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());

	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[swapChain_->GetSwapChain()->GetCurrentBackBufferIndex()], false, &dsvHandle);
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexModel_++;
}
void Engine::DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color, const WorldTransform& uvTransform)
{
	// indexModel_が範囲内であることを確認
	assert(indexModel_ < kMaxModelCount);

	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するモデルを取得
	RenderData& model = models_[indexModel_];

	// 一度だけ作られたMeshを使う
	Mesh* mesh = GetOrCreateMesh(modelData);
	model.mesh = *mesh;

	// 色変換
	modelData.materialHandle = model.materialHandle;
	modelData.materialHandle.materialData->color = Uint32ToColorVector(color);

	// ワールド行列 (スケール・回転・位置) を計算
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列 (World * ViewProjection) を計算
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// uvTransformMatrixの設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	// マテリアルリソースに uvTransformMatrix を設定
	model.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// パイプラインステートの設定
	if (isWireFrame_) {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3DWireframe_.Get());
	}
	else {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	}
	// プリミティブ形状の設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &model.mesh.GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&model.mesh.GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, modelData.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());

	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[swapChain_->GetSwapChain()->GetCurrentBackBufferIndex()], false, &dsvHandle);
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(model.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexModel_++;
}

void Engine::DrawModel(WorldTransform& worldTransform, Camera& camera, const AnimatedModelData& instance, uint32_t textureHandle, uint32_t color)
{
	assert(indexModel_ < kMaxModelCount);
	RenderData& model = models_[indexModel_];
	Mesh* mesh = GetOrCreateMesh(instance.modelData);

	// 色変換
	instance.modelData.materialHandle.materialData->color = Uint32ToColorVector(color);

	// アニメーションによる変換行列(localMatrix)を使ってワールド行列を計算
	model.worldMatrix = instance.localMatrix * worldTransform.matWorld_;

	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// パイプラインステートの設定
	if (isWireFrame_) {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3DWireframe_.Get());
	}
	else {
		commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	}
	// プリミティブ形状の設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, instance.modelData.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());

	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[swapChain_->GetSwapChain()->GetCurrentBackBufferIndex()], false, &dsvHandle);
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexModel_++;
}

void Engine::DrawGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color)
{
	// indexModel_が範囲内であることを確認
	assert(indexModel_ < kMaxModelCount);


	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するモデルを取得
	RenderData& model = models_[indexModel_];

	// 一度だけ作られたMeshを使う
	Mesh* mesh = GetOrCreateMesh(modelData);

	// 色変換
	modelData.materialHandle.materialData->color = Uint32ToColorVector(color);

	// ワールド行列 (スケール・回転・位置) を計算
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列 (World * ViewProjection) を計算
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->psoGrid_.Get());
	// プリミティブ形状の設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, modelData.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());

	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvManager_->rtvHandles[swapChain_->GetSwapChain()->GetCurrentBackBufferIndex()], false, &dsvHandle);
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexModel_++;
}

void Engine::CreateSprites()
{
	// 最大数のスプライト分の配列を確保
	sprites_.resize(kMaxSpriteCount);

	// 原点を左上にするように設定
	std::vector<VertexData> spriteVertices = {
		// 左上
		{{0.0f,       0.0f,        0.0f, 1.0f}, {0.0f, 0.0f}},
		// 右上
		{{1.0f,      0.0f,        0.0f, 1.0f}, {1.0f, 0.0f}},
		// 左下
		{{0.0f,       1.0f,      0.0f, 1.0f}, {0.0f, 1.0f}},
		// 右下
		{{1.0f,      1.0f,      0.0f, 1.0f}, {1.0f, 1.0f}},
	};

	// 初期仮のインデックス(これも後で実質的に無視される)
	std::vector<uint32_t> spriteIndices = { 0,1,2,1,3,2 };

	// 指定数分のスフィアメッシュとリソースを初期化
	for (size_t i = 0; i < kMaxSpriteCount; ++i)
	{
		// メッシュ初期化(vertex + index データをGPUへ転送)
		sprites_[i].mesh.Initialize(graphicDevice_->GetDevice(), spriteVertices, spriteIndices);

		// マテリアルを作成・設定
		sprites_[i].materialHandle = materialManager_->CreateMaterial(graphicDevice_->GetDevice());
		sprites_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();// UV行列は単位行列で初期化
		// WVP行列用のバッファを作成
		sprites_[i].wvpResource = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		sprites_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&sprites_[i].mappedData));  // CPUアクセス用にマッピング

		sprites_[i].mesh.SetIndexCount(spriteIndices.size()); // インデックス数を設定
	}

	// 最初に使用するスフィアのインデックスをリセット
	indexSprite_ = 0;
}

void Engine::DrawSprite(Vector2 position, Vector2 size, float rotation, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	// indexSprite_が範囲外でないことを確認
	assert(indexSprite_ < kMaxSpriteCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するスプライトを取得
	RenderData& sprite = sprites_[indexSprite_];

	// マテリアルに色情報を設定
	sprite.materialHandle.materialData->color = Uint32ToColorVector(color);

	sprite.materialHandle.materialData->enableLighting = false;

	// スプライトのワールド行列を計算
	Matrix4x4 scaleMatrix = Matrix4x4::MakeScale({ size.x, size.y, 1.0f });
	Matrix4x4 rotationMatrix = Matrix4x4::MakeRotateZ(rotation);
	Matrix4x4 translateMatrix = Matrix4x4::MakeTranslate({ position.x, position.y, 0.0f });

	// WorldTransformのメンバーを直接使う代わりに、ローカル変数で計算
	sprite.worldMatrix = (scaleMatrix * rotationMatrix) * translateMatrix;

	// MakeWVPMatrix2D関数に直接Matrix4x4を渡せるようにする
	// worldMatrixから一時的なWorldTransformを構築してMakeWVPMatrix2Dに渡す
	WorldTransform tempTransform = {
		{size.x, size.y, 1.0f},
		{0.0f, 0.0f, rotation},
		{position.x, position.y, 0.0f}
	};
	Matrix4x4 wvpMatrix = Matrix4x4::MakeWVPMatrix2D(tempTransform, float(kClientWidth), float(kClientHeight));

	// WVP行列をGPU用バッファにコピー
	memcpy(&sprite.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));

	// uvTransformMatrixの設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	// マテリアルリソースに uvTransformMatrix を設定
	sprite.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// プリミティブ形状を設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&sprite.mesh.GetIndexBufferView());
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &sprite.mesh.GetVertexBufferView());
	// 定数バッファ(RootParameter)をGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sprite.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sprite.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexSprite_++;
}

void Engine::CreateCubes()
{
	cubes_.resize(kMaxCubeCount);

	// 立方体の頂点とインデックスを生成
	std::vector<VertexData> cubeVertices;
	std::vector<uint32_t> cubeIndices;

	ShapeGenerator shapeGenerator;
	shapeGenerator.CubeGenerator(cubeVertices, cubeIndices); 

	for (size_t i = 0; i < kMaxCubeCount; ++i) 
	{
		// メッシュ初期化
		cubes_[i].mesh.Initialize(graphicDevice_->GetDevice(), cubeVertices, cubeIndices);

		// マテリアル作成
		cubes_[i].materialHandle = materialManager_->CreateMaterial(graphicDevice_->GetDevice());
		cubes_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();

		// WVPバッファ作成
		cubes_[i].wvpResource = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		cubes_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&cubes_[i].mappedData));

		cubes_[i].mesh.SetIndexCount(cubeIndices.size());
	}

	indexCube_ = 0;
}
void Engine::DrawCube(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	// indexCube_が範囲内であることを確認
	assert(indexCube_ < kMaxCubeCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する立方体を取得
	RenderData& cube = cubes_[indexCube_];

	// 色変換
	cube.materialHandle.materialData->color = Uint32ToColorVector(color);;

	// ワールド行列 (スケール・回転・位置) を計算
	cube.worldMatrix = Matrix4x4::MakeAffine(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

	// WVP行列 (World * ViewProjection) を計算
	camera_->UpdateViewProjectionMatrix();
	Matrix4x4 wvpMatrix = cube.worldMatrix * camera_->GetViewProjectionMatrix();
	memcpy(&cube.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	cube.mappedData->World = cube.worldMatrix;
	cube.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(cube.worldMatrix.Transpose());

	// UV変換行列の設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	cube.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->pso3D_.Get());
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignature3D_.Get());
	// 頂点バッファの設定
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &cube.mesh.GetVertexBufferView());
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&cube.mesh.GetIndexBufferView());
	// 定数バッファをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, cube.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, cube.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, textures_[textureHandle].srvManager.GetSrvHandleGPU());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(cube.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexCube_++;
}

void Engine::CreateLines()
{
	lines_.resize(kMaxLineCount);

	for (size_t i = 0; i < kMaxLineCount; ++i)
	{
		lines_[i].materialHandle = materialManager_->CreateLineMaterial(graphicDevice_->GetDevice());

		lines_[i].wvpResource = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(TransformationMatrix));
		lines_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&lines_[i].mappedData));
		lines_[i].mesh.SetVertexCount(2);

		// 空の2頂点バッファを1回だけ作る（後でMap更新）
		std::vector<VertexData> dummyVertices = {
			{}, {}
		};
		lines_[i].mesh.InitializeVertexOnly(graphicDevice_->GetDevice(), dummyVertices);
	}
	indexLine_ = 0;
}

void Engine::DrawLine(const Vector3& start, const Vector3& end, Camera& camera, uint32_t color)
{
	assert(indexLine_ < kMaxLineCount);

	// 描画に必要なSRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& line = lines_[indexLine_];

	// 頂点バッファに直接書き込む（毎回作らない）
	VertexData* mappedVertices = nullptr;
	line.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));

	mappedVertices[0] = { { start.x, start.y, start.z, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
	mappedVertices[1] = { { end.x,   end.y,   end.z,   1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

	line.mesh.GetVertexResource()->Unmap(0, nullptr);
	line.mesh.SetVertexCount(2);

	// マテリアル色のみ更新
	line.materialHandle.lineMaterialData->color = Uint32ToColorVector(color);

	// WVP行列更新
	line.worldMatrix = Matrix4x4::MakeIdentity();
	Matrix4x4 wvpMatrix = line.worldMatrix * camera.GetViewProjectionMatrix();
	memcpy(&line.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));

	// パイプライン設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->psoLine_.Get());
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureLine_.Get());

	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &line.mesh.GetVertexBufferView());

	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, line.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, line.wvpResource->GetGPUVirtualAddress());

	commandManager_->GetCommandList()->DrawInstanced(UINT(line.mesh.GetVertexCount()), 1, 0, 0);

	indexLine_++;
}


void Engine::CreateParticles()
{
	// 最大数のパーティクル分の配列を確保
	particles_.resize(kMaxParticleCount);

	// 頂点・インデックス(1枚の板ポリ)
	std::vector<VertexData> vertices =
	{
		{{-0.5f, -0.5f, 0, 1}, {0, 1}, {0, 0, -1}},
		{{0.5f, -0.5f, 0, 1}, {1, 1}, {0, 0, -1}},
		{{-0.5f, 0.5f, 0, 1}, {0, 0}, {0, 0, -1}},
		{{0.5f, 0.5f, 0, 1}, {1, 0}, {0, 0, -1}},
	};
	std::vector<uint32_t> indices = { 0, 1, 2, 1, 3, 2 };

	particleMesh_.Initialize(graphicDevice_->GetDevice(), vertices, indices);

	// インスタンスバッファをフレーム数分リングで確保
	for (int i = 0; i < kFrameCount; ++i)
	{
		particleInstanceBuffer_[i] = BufferManager::CreateBufferResource(
			graphicDevice_->GetDevice(),
			sizeof(ParticleInstanceData) * kMaxParticleCount);

		particleInstanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_[i]));
	}

	cameraBuffer_ = BufferManager::CreateBufferResource(graphicDevice_->GetDevice(), sizeof(CameraBuffer));
	cameraBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedCamera_));

	psoManager_->CreateAllParticlePipelines();
}


void Engine::SubmitParticleInstance(WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ)
{
	if (indexInstance_ >= kMaxParticleCount) return;

	ParticleInstanceData& data = mappedInstanceData_[currentFrameIndex_][indexInstance_++];
	data.worldMatrix = worldTransform.matWorld_;

	data.color = Uint32ToColorVector(color);
	data.textureIndex = textureIndex;
	data.rotationZ = rotationZ;
	indexParticle_++;

	particlesByTexture_[textureIndex].push_back(data);
}

void Engine::DrawParticles(const Camera& camera)
{
	if (indexInstance_ == 0) return;

	auto* cmdList = commandManager_->GetCommandList();

	// 共通設定（パイプラインやルートシグネチャ、頂点/indexバッファ）
	auto& psoMap = psoManager_->psoParticles_;
	auto it = psoMap.find(currentBlendMode_);
	if (it == psoMap.end()) return;

	cmdList->SetPipelineState(it->second.Get());
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureParticles_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetIndexBuffer(&particleMesh_.GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &particleMesh_.GetVertexBufferView());

	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// カメラ定数バッファ更新
	mappedCamera_->viewProjectionMatrix = camera.GetViewProjectionMatrix();
	Matrix4x4 view = camera.GetViewMatrix();
	mappedCamera_->cameraRight = { view.m[0][0], view.m[1][0], view.m[2][0] };
	mappedCamera_->cameraUp = { view.m[0][1], view.m[1][1], view.m[2][1] };
	cmdList->SetGraphicsRootConstantBufferView(1, cameraBuffer_->GetGPUVirtualAddress());

	// 先頭アドレス
	ParticleInstanceData* dstBase = mappedInstanceData_[currentFrameIndex_];

	// インスタンスデータのオフセット
	size_t offset = 0;

	for (auto& [textureIndex, instances] : particlesByTexture_)
	{
		if (instances.empty()) continue;

		// コピー先をずらしてセット
		ParticleInstanceData* dst = dstBase + offset;
		memcpy(dst, instances.data(), sizeof(ParticleInstanceData) * instances.size());

		// テクスチャのSRVをセット
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = textures_[textureIndex].srvManager.GetSrvHandleGPU();
		cmdList->SetGraphicsRootDescriptorTable(3, srvHandle);

		// インスタンスバッファのGPUアドレスにオフセットを加算してセット
		UINT64 gpuAddress = particleInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
		gpuAddress += sizeof(ParticleInstanceData) * offset;
		cmdList->SetGraphicsRootShaderResourceView(0, gpuAddress);

		// 描画
		cmdList->DrawIndexedInstanced(
			static_cast<UINT>(particleMesh_.GetIndexCount()),
			static_cast<UINT>(instances.size()),
			0, 0, 0);

		offset += instances.size();
	}

	particlesByTexture_.clear();
	currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
	indexInstance_ = 0;
}