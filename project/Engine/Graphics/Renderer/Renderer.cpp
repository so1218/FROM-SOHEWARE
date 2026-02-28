#include "Renderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "GlobalConstants.h"
#include "MaterialManager.h"
#include "BufferManager.h"
#include "ShapeGenerator.h"
#include "Camera.h"
#include "PostEffectManager.h"
#include "TextureManager.h"
#include "TimeManager.h"

// 最大数の定義
const int32_t Renderer::kMaxParticleCount = 8000;// パーティクルの最大数
const int32_t Renderer::kMaxTrailCount = 300; // 同時に描画できるトレイルの最大本数
const int32_t Renderer::kMaxTrailVertices = 512; // 1つのトレイルの最大頂点数

Renderer::Renderer() {}
Renderer::~Renderer()
{

}

void Renderer::Initialize(
	GraphicsDevice* device, CommandManager* commandManager,
	PSOManager* psoManager, RootSignatureManager* rootSignatureManager,
	TextureLoader* textureLoader, SRVManager* srvManager, LightManager* lightManager,
	GlobalConstants* globalConstants, MaterialManager* materialManager,
	PostEffectManager* postEffectManager,
	int clientWidth, int clientHeight, ShadowMap* shadowMap)
{
	// ポインタをメンバ変数に保存
	device_ = device;
	commandManager_ = commandManager;
	psoManager_ = psoManager;
	rootSignatureManager_ = rootSignatureManager;
	textureLoader_ = textureLoader;
	srvManager_ = srvManager;
	lightManager_ = lightManager;
	globalConstants_ = globalConstants;
	materialManager_ = materialManager;
	postEffectManager_ = postEffectManager;

	env_.device = device_;
	env_.commandManager = commandManager_;
	env_.psoManager = psoManager_;
	env_.rootSignatureManager = rootSignatureManager_;
	env_.textureLoader = textureLoader_;
	env_.srvManager = srvManager_;
	env_.lightManager = lightManager_;
	env_.globalConstants = globalConstants_;
	env_.materialManager = materialManager_;
	env_.postEffectManager = postEffectManager_;

	modelRenderer_ = std::make_unique<ModelRenderer>();
	modelRenderer_->Initialize(env_);
	spriteRenderer_ = std::make_unique<SpriteRenderer>();
	spriteRenderer_->Initialize(env_, clientWidth, clientHeight);
	lineRenderer_ = std::make_unique<LineRenderer>();
	lineRenderer_->Initialize(env_);

	viewMatrix_ = Matrix4x4::MakeIdentity();
	projectionMatrix_ = Matrix4x4::MakeIdentity();
	viewProjectionMatrix_ = Matrix4x4::MakeIdentity();

	CreateObjects();

	shadowMap_ = shadowMap;
}

void Renderer::Finalize()
{
	modelRenderer_->Finalize();
	spriteRenderer_->Finalize();
}

void Renderer::BeginFrame()
{
	prevParticleCount_ = indexParticle_;
	prevTrailCount_ = indexTrail_;

	// 描画カウンタの初期化
	indexParticle_ = 0;
	indexInstance_ = 0;
	indexTrail_ = 0;

	if (modelRenderer_) { modelRenderer_->BeginFrame(); }
	if (spriteRenderer_) { spriteRenderer_->BeginFrame(); }
	if (lineRenderer_) { lineRenderer_->BeginFrame(); }
}

void Renderer::CreateObjects()
{
	CreateParticles();
	CreateSkybox();
	CreateTrails();
}

void Renderer::SetCameraState(const Matrix4x4& view, const Matrix4x4& projection, const Vector3& cameraPosition)
{
	viewMatrix_ = view;
	projectionMatrix_ = projection;
	viewProjectionMatrix_ = view * projection;
	cameraPosition_ = cameraPosition;

	if (modelRenderer_)
	{
		modelRenderer_->SetCameraState(viewMatrix_, viewProjectionMatrix_);
	}
}

int Renderer::LoadTexture(const std::string& texturePath)
{
	// テクスチャをロード
	DirectX::ScratchImage mipImages = TextureLoader::LoadTexture(texturePath);

	// テクスチャをアップロード
	TextureLoader::TextureResources texResources = textureLoader_->UploadTexture(mipImages);

	// 保存したテクスチャのインデックスを返す
	return texResources.srvIndex;
}

void Renderer::LoadTextureArray(const std::vector<std::string>& texturePaths)
{
	// 複数テクスチャをロード
	std::vector<DirectX::ScratchImage> images = textureLoader_->LoadMultipleTextures(texturePaths);

	// Texture2DArray作成＆アップロード
	textureLoader_->CreateAndUploadTexture2DArray(images, textureArrayResource_);

}

void Renderer::DrawFullScreenQuadWithOffscreenTexture()
{
	auto* cmdList = commandManager_->GetCommandList();

	// SRVヒープをセット（ポストプロセス入力用）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	// フルスクリーン用PSO / ルートシグネチャ
	cmdList->SetPipelineState(psoManager_->GetPSO("Fullscreen"));
	cmdList->SetGraphicsRootSignature(
		rootSignatureManager_->GetRootSignature("Fullscreen")
	);

	// ポストエフェクト定数バッファ
	cmdList->SetGraphicsRootConstantBufferView(
		0,
		postEffectManager_->GetPostEffectDataAddress()
	);

	// 最終入力テクスチャ（Bloom合成結果）
	uint32_t finalImageIndex = postEffectManager_->GetBloomCombineSRVIndex();
	cmdList->SetGraphicsRootDescriptorTable(
		1,
		srvManager_->GetSRVHandleGPU(finalImageIndex)
	);

	uint32_t dissolveMapIndex = TextureManager::GetInstance().Get("noise_01");
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		srvManager_->GetSRVHandleGPU(dissolveMapIndex)
	);

	// 現在のLUTの名前を取得
	std::string lutName = postEffectManager_->GetCurrentLutName();
	uint32_t lutMapIndex = TextureManager::GetInstance().Get(lutName);

	cmdList->SetGraphicsRootDescriptorTable(
		3, srvManager_->GetSRVHandleGPU(lutMapIndex)
	);

	// フルスクリーントライアングル描画
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

// 単純にテクスチャをそのまま画面に出すメソッド
void Renderer::DrawFinalResult(uint32_t srvIndex)
{
	auto* cmdList = commandManager_->GetCommandList();

	// SRVヒープをセット（最終出力テクスチャ）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(1, heaps);

	// フルスクリーン描画用PSO / ルートシグネチャ
	cmdList->SetPipelineState(psoManager_->GetPSO("Fullscreen"));
	cmdList->SetGraphicsRootSignature(
		rootSignatureManager_->GetRootSignature("Fullscreen")
	);

	// ポストエフェクト定数
	cmdList->SetGraphicsRootConstantBufferView(
		0,
		postEffectManager_->GetPostEffectDataAddress()
	);

	// 最終結果テクスチャ
	cmdList->SetGraphicsRootDescriptorTable(
		1,
		srvManager_->GetSRVHandleGPU(srvIndex)
	);

	uint32_t dissolveMapIndex = TextureManager::GetInstance().Get("noise_01");
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		srvManager_->GetSRVHandleGPU(dissolveMapIndex)
	);

	// 現在のLUTの名前を取得
	std::string lutName = postEffectManager_->GetCurrentLutName();
	uint32_t lutMapIndex = TextureManager::GetInstance().Get(lutName);

	cmdList->SetGraphicsRootDescriptorTable(
		3, srvManager_->GetSRVHandleGPU(lutMapIndex)
	);

	// フルスクリーントライアングル描画
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}

void Renderer::DrawSceneForShadow()
{
	auto* cmdList = commandManager_->GetCommandList();
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	modelRenderer_->DrawShadow(env_);
}

void Renderer::Draw3D()
{
	// トレイル描画を登録
	if (!trailBatch_.verticesCPU.empty())
	{
		ModelSubmission trailSubmission{};
		trailSubmission.type = RenderType::Trail;
		trailSubmission.group = RenderGroup::Trail; // 半透明グループ
		trailSubmission.depth = 0.0f; // 深度
		modelSubmissions_.push_back(trailSubmission);
	}

	// パーティクル描画を登録
	if (hasParticles_)
	{
		ModelSubmission particleSubmission{};
		particleSubmission.type = RenderType::Particle;
		particleSubmission.group = RenderGroup::Particle;
		particleSubmission.depth = 0.0f;
		modelSubmissions_.push_back(particleSubmission);
	}

	// 描画順にソート（グループ→深度→UI順）
	std::sort(modelSubmissions_.begin(), modelSubmissions_.end(),
		[](const ModelSubmission& a, const ModelSubmission& b)
		{
			if (a.group != b.group) return a.group < b.group;
			switch (a.group)
			{
			case RenderGroup::Opaque: return a.depth < b.depth;
			case RenderGroup::Grid: return a.depth < b.depth;
			case RenderGroup::Transparent: return a.depth > b.depth;
			default: return a.depth < b.depth;
			}
		});

	auto* cmdList = commandManager_->GetCommandList();

	// 共通設定
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 不透明モデルをまとめて描画
	if (modelRenderer_)
	{
		modelRenderer_->Draw(env_, RenderGroup::Opaque, isWireFrame_, shadowMap_);
	}

	if (modelRenderer_) 
	{
		modelRenderer_->Draw(env_, RenderGroup::Grid, isWireFrame_, shadowMap_);
	}

	// モデル以外のもの（スプライト、ライン等）を描画
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (const auto& sub : modelSubmissions_)
	{
		// モデルは描画済みなのでスキップ
		if (sub.type == RenderType::Model || sub.type == RenderType::Skinning) continue;

		// 半透明系(Particle, Trail)は後で描画するのでスキップ
		if (sub.group == RenderGroup::Particle || sub.group == RenderGroup::Transparent || sub.group == RenderGroup::Trail) continue;

		switch (sub.type) 
		{
		case RenderType::Skybox: DrawSkybox(sub); break;
		}
	}

	if (lineRenderer_)
	{
		lineRenderer_->Draw(env_, viewProjectionMatrix_);
	}

	// 半透明モデルをまとめて描画
	if (modelRenderer_)
	{
		modelRenderer_->Draw(env_, RenderGroup::Transparent, isWireFrame_, shadowMap_);
	}

	for (const auto& sub : modelSubmissions_)
	{
		if (sub.type == RenderType::Model || sub.type == RenderType::Skinning) continue;

		// ここでは半透明系のみを描画
		switch (sub.type) 
		{
		case RenderType::Particle: DrawParticles(); break;
		case RenderType::Trail:    DrawTrails();    break;
		}
	}
}

void Renderer::DrawUI()
{
	auto* cmdList = commandManager_->GetCommandList();

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// UIの描画はSpriteRenderer
	if (spriteRenderer_)
	{
		spriteRenderer_->Draw(env_);
	}

	// 後処理
	modelSubmissions_.clear();
	particleBatches_.clear();
	trailBatch_.verticesCPU.clear();
	trailBatches_.clear();
	hasParticles_ = false;
	currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

void Renderer::SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
	const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
	DepthMode depthMode, RenderGroup group, const Vector4& instanceColor)
{
	if (modelRenderer_)
	{
		modelRenderer_->Submit(worldTransform, modelData, materials, blendMode, cullMode, depthMode, group, instanceColor);
	}
}

void Renderer::SubmitAnimationModel(
	const WorldTransform& worldTransform,
	const AnimatedModelData& instance,
	const SkinCluster& skinCluster,
	const std::vector<MaterialHandle>& materials,
	BlendMode blendMode,
	RenderGroup group,
	const Vector4& instanceColor)
{
	if (modelRenderer_)
	{
		modelRenderer_->SubmitAnimation(
			worldTransform, instance, skinCluster, materials, blendMode, group, instanceColor
		);
	}
}

void Renderer::SubmitSprite(const Vector2 position, const Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, const WorldTransform& uvTransform,
	uint32_t textureHandle, uint32_t dissolveTextureHandle, int layerOrder,
	const MaterialHandle& materialHandle)
{
	if (spriteRenderer_)
	{
		spriteRenderer_->Submit(
			position, size, rotation, color, anchorPoint, uvTransform,
			textureHandle, dissolveTextureHandle, layerOrder, materialHandle
		);
	}
}

void Renderer::SubmitLine(const Vector3& start, const Vector3& end, uint32_t color)
{
	if (lineRenderer_) { lineRenderer_->Submit(start, end, color); }
}

void Renderer::CreateParticles()
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

	particleMesh_.Initialize(device_->GetDevice(), vertices, indices);

	// インスタンスバッファをフレーム数分リングで確保
	for (int i = 0; i < kFrameCount; ++i)
	{
		particleInstanceBuffer_[i] = BufferManager::CreateBufferResource(
			device_->GetDevice(),
			sizeof(ParticleInstanceData) * kMaxParticleCount);

		particleInstanceBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData_[i]));
	}
}

void Renderer::SubmitParticleInstance(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
	BlendMode blendMode, bool isBillboard, float intensity)
{
	if (indexInstance_ >= kMaxParticleCount) return;

	// インスタンスデータ作成
	ParticleInstanceData data;
	data.worldMatrix = worldTransform.matWorld_;
	data.color = Math::Uint32ToColorVector(color);
	data.textureIndex = textureIndex;
	data.rotationZ = rotationZ;
	data.isBillboard = isBillboard ? 1 : 0;
	data.intensity = intensity;

	// ブレンドモード・テクスチャごとにバッチ登録
	particleBatches_[blendMode][textureIndex].push_back(data);

	indexParticle_++;
	indexInstance_++;

	// このフレームでパーティクル描画が必要であることを記録
	hasParticles_ = true;
}

void Renderer::DrawParticles()
{
	if (indexInstance_ == 0) return;

	auto* cmdList = commandManager_->GetCommandList();

	// ルートシグネチャとトポロジー設定
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Particle"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点・インデックスバッファセット
	cmdList->IASetIndexBuffer(&particleMesh_.GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &particleMesh_.GetVertexBufferView());

	// カメラ情報を更新
	cmdList->SetGraphicsRootConstantBufferView(1, globalConstants_->GetResource()->GetGPUVirtualAddress());

	ParticleInstanceData* dstBase = mappedInstanceData_[currentFrameIndex_];
	size_t currentOffset = 0;

	// ブレンドモードごとに描画
	for (auto& [blendMode, textureMap] : particleBatches_)
	{
		std::string psoName;
		switch (blendMode)
		{
		case kBlendModeNone:      psoName = "ParticleOpaque"; break;
		case kBlendModeNormal:    psoName = "ParticleAlphaBlend"; break;
		case kBlendModeAdd:       psoName = "ParticleAdditive"; break;
		case kBlendModeSubtract:  psoName = "ParticleSubtract"; break;
		case kBlendModeMultiply:  psoName = "ParticleMultiply"; break;
		case kBlendModeScreen:    psoName = "ParticleScreen"; break;
		case kBlendModeExclusion: psoName = "ParticleExclusion"; break;
		default:                  psoName = "ParticleAlphaBlend"; break;
		}

		ID3D12PipelineState* pso = psoManager_->GetPSO(psoName);
		if (!pso) continue;
		cmdList->SetPipelineState(pso);

		// テクスチャごとに描画
		for (auto& [textureIndex, instances] : textureMap)
		{
			if (instances.empty()) continue;

			// インスタンスデータをGPUにコピー
			ParticleInstanceData* dst = dstBase + currentOffset;
			memcpy(dst, instances.data(), sizeof(ParticleInstanceData) * instances.size());

			// テクスチャSRVセット
			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvManager_->GetSRVHandleGPU(textureIndex);
			cmdList->SetGraphicsRootDescriptorTable(3, srvHandle);

			// インスタンスバッファセット
			UINT64 gpuAddress = particleInstanceBuffer_[currentFrameIndex_]->GetGPUVirtualAddress();
			gpuAddress += sizeof(ParticleInstanceData) * currentOffset;
			cmdList->SetGraphicsRootShaderResourceView(0, gpuAddress);

			// 描画
			cmdList->DrawIndexedInstanced(
				static_cast<UINT>(particleMesh_.GetIndexCount()),
				static_cast<UINT>(instances.size()),
				0, 0, 0);

			currentOffset += instances.size();
		}
	}
}

void Renderer::CreateSkybox()
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	// ShapeGeneratorを使ってメッシュデータを生成
	ShapeGenerator::SkyBoxGenerator(vertices, indices);

	// メッシュを初期化 (GPUにデータを転送)
	skyboxMesh_.Initialize(device_->GetDevice(), vertices, indices);

	// WVP行列用のバッファを作成
	skyboxWvpResource_ = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
	skyboxWvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedSkyboxWvp_));

	// マテリアルバッファを作成
	skyboxMaterialHandle_ = materialManager_->CreateMaterial(device_->GetDevice());

	// スカイボックスのデフォルト色
	skyboxMaterialHandle_.materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void Renderer::SubmitSkybox(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
	// WVP行列の計算（カメラの位置を除去して回転のみ反映）
	Matrix4x4 viewMatrix = viewMatrix_;
	Matrix4x4 projectionMatrix = projectionMatrix_;
	viewMatrix.m[3][0] = 0.0f;
	viewMatrix.m[3][1] = 0.0f;
	viewMatrix.m[3][2] = 0.0f;

	Matrix4x4 worldMatrix = worldTransform.matWorld_;
	Matrix4x4 wvpMatrix = worldMatrix * viewMatrix_ * projectionMatrix_;
	memcpy(mappedSkyboxWvp_, &wvpMatrix, sizeof(TransformationMatrix));

	// マテリアルカラー設定
	skyboxMaterialHandle_.materialData->color = Math::Uint32ToColorVector(color);

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Skybox;
	submission.group = RenderGroup::Opaque;

	// 深度を最大値にして必ず最後に描画
	submission.depth = FLT_MAX;

	// キューブテクスチャを指定
	submission.textureHandle = cubeTextureSrvIndex;

	modelSubmissions_.push_back(submission);
}

void Renderer::CreateTrails()
{
	// 最大Trail数分の頂点を確保
	const uint32_t kMaxTotalTrailVertices =
		kMaxTrailCount * kMaxTrailVertices * 2;
	trailBatch_.verticesCPU.reserve(kMaxTotalTrailVertices);

	// GPU頂点バッファ確保（ダミー）
	std::vector<VertexDataTrail> dummyVertices(kMaxTotalTrailVertices);
	trailBatch_.mesh.InitializeVertexTrail(device_->GetDevice(), dummyVertices);

	// マテリアルCB
	uint32_t materialSize = sizeof(TrailMaterialData);
	materialSize = (materialSize + 255) & ~255;

	trailBatch_.materialResource =
		BufferManager::CreateBufferResource(
			device_->GetDevice(), materialSize * kMaxTrailCount);
	trailBatch_.materialResource->Map(
		0, nullptr,
		reinterpret_cast<void**>(&trailBatch_.mappedMaterial));

	// ViewProjectionのみ
	trailBatch_.wvpResource =
		BufferManager::CreateBufferResource(
			device_->GetDevice(), sizeof(TransformationMatrix));
	trailBatch_.wvpResource->Map(
		0, nullptr,
		reinterpret_cast<void**>(&trailBatch_.mappedWvp));
}

void Renderer::SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config)
{
	// 上限・最小チェック
	if (indexTrail_ >= kMaxTrailCount) return;
	if (points.size() < 2) return;

	// テクスチャ取得
	uint32_t textureHandle = TextureManager::GetInstance().Get(config.textureName);
	uint32_t dissolveHandle = 0;
	if (!config.dissolveTextureName.empty() && config.dissolveTextureName != "none") 
	{
		dissolveHandle = TextureManager::GetInstance().Get(config.dissolveTextureName);
	}
	else
	{
		dissolveHandle = TextureManager::GetInstance().Get("white1x1");
	}

	// マテリアル定数
	TrailMaterialData currentMatData{};
	currentMatData.scrollSpeed = config.scrollSpeed;
	currentMatData.jitterStrength = config.jitterStrength;
	currentMatData.jitterFrequency = config.jitterFrequency;
	currentMatData.jitterSpeed = config.jitterSpeed;
	currentMatData.jitterMode = static_cast<int>(config.jitterMode);
	currentMatData.jitterPhase = config.jitterPhase;
	currentMatData.isDissolveEnabled = (config.dissolveTextureName != "white1x1") ? 1 : 0;
	currentMatData.emissiveIntensity = config.emissiveIntensity;

	// バッチ切り替え判定
	bool isNewBatch = trailBatches_.empty();
	if (!isNewBatch)
	{
		const auto& last = trailBatches_.back();
		// memcmp は危険な場合もありますが（パディング等）、とりあえずそのまま
		isNewBatch =
			last.textureHandle != textureHandle ||
			last.dissolveHandle != dissolveHandle ||
			std::memcmp(&last.materialData, &currentMatData, sizeof(TrailMaterialData)) != 0;
	}

	if (isNewBatch)
	{
		trailBatches_.push_back({
			static_cast<uint32_t>(trailBatch_.verticesCPU.size()),
			0,
			textureHandle,
			dissolveHandle,
			currentMatData
			});
	}

	// Tile用距離
	std::vector<float> distances;
	if (config.textureMode == TrailTextureMode::Tile)
	{
		distances.resize(points.size());
		float total = 0.0f;
		for (size_t i = 0; i < points.size() - 1; ++i)
		{
			distances[i] = total;
			total += (points[i + 1].position - points[i].position).Length();
		}
		distances.back() = total;
	}

	Vector3 cameraPos = cameraPosition_;

	// セグメント生成
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		auto CalcVertex = [&](size_t idx, float& outU)
			{
				const Vector3& pos = points[idx].position;

				Vector3 forward =
					(idx < points.size() - 1)
					? points[idx + 1].position - pos
					: pos - points[idx - 1].position;
				forward = forward.Normalize();

				Vector3 right;
				if (config.alignment == TrailAlignment::View)
				{
					Vector3 toCamera = (cameraPos - pos).Normalize();
					right = Math::CrossProduct(toCamera, forward).Normalize();
				}
				else
				{
					Vector3 up = points[idx].rotationQuaternion.RotateVector({ 0,1,0 });
					right = Math::CrossProduct(up, forward).Normalize();
				}
				if (right.LengthSq() < 0.001f)
					right = Math::CrossProduct({ 0,1,0 }, forward).Normalize();

				float t = static_cast<float>(idx) / (points.size() - 1);
				float width = config.width *
					std::lerp(config.tailWidthScale, config.headWidthScale, t);

				outU = (config.textureMode == TrailTextureMode::Stretch)
					? t * config.tiling.x
					: distances[idx] * config.tiling.x;

				return std::pair(
					pos - right * (width * 0.5f),
					pos + right * (width * 0.5f)
				);
			};

		auto CalcColor = [&](size_t idx)
			{
				float t = static_cast<float>(idx) / (points.size() - 1);
				return Vector4(
					std::lerp(config.endColor.x, config.startColor.x, t),
					std::lerp(config.endColor.y, config.startColor.y, t),
					std::lerp(config.endColor.z, config.startColor.z, t),
					std::lerp(config.endColor.w, config.startColor.w, t)
				);
			};

		float u0, u1;
		auto [l0, r0] = CalcVertex(i, u0);
		auto [l1, r1] = CalcVertex(i + 1, u1);
		Vector4 c0 = CalcColor(i);
		Vector4 c1 = CalcColor(i + 1);

		// Quad → TriangleList
		trailBatch_.verticesCPU.push_back({ {l0.x,l0.y,l0.z,1}, {u0,0}, c0 });
		trailBatch_.verticesCPU.push_back({ {l1.x,l1.y,l1.z,1}, {u1,0}, c1 });
		trailBatch_.verticesCPU.push_back({ {r0.x,r0.y,r0.z,1}, {u0,1}, c0 });

		trailBatch_.verticesCPU.push_back({ {r0.x,r0.y,r0.z,1}, {u0,1}, c0 });
		trailBatch_.verticesCPU.push_back({ {l1.x,l1.y,l1.z,1}, {u1,0}, c1 });
		trailBatch_.verticesCPU.push_back({ {r1.x,r1.y,r1.z,1}, {u1,1}, c1 });

		trailBatches_.back().vertexCount += 6;
	}

	indexTrail_++;
}

void Renderer::DrawTrails()
{
	if (trailBatches_.empty() || trailBatch_.verticesCPU.empty()) return;

	auto* cmdList = commandManager_->GetCommandList();

	// 頂点データの転送
	VertexDataTrail* mappedVertices = nullptr;
	trailBatch_.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));
	memcpy(mappedVertices, trailBatch_.verticesCPU.data(), sizeof(VertexDataTrail) * trailBatch_.verticesCPU.size());
	trailBatch_.mesh.GetVertexResource()->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = trailBatch_.mesh.GetVertexBufferView();
	vbView.SizeInBytes = static_cast<UINT>(sizeof(VertexDataTrail) * trailBatch_.verticesCPU.size());
	cmdList->IASetVertexBuffers(0, 1, &vbView);

	// パイプライン設定
	cmdList->SetPipelineState(psoManager_->GetPSO("Trail"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Trail"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 全バッチ共通のWVP更新
	trailBatch_.mappedWvp->WVP = viewProjectionMatrix_;
	trailBatch_.mappedWvp->World = Matrix4x4::MakeIdentity();
	cmdList->SetGraphicsRootConstantBufferView(0, trailBatch_.wvpResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(2, globalConstants_->GetResource()->GetGPUVirtualAddress());

	// バッチ描画
	const uint32_t alignedSize = (sizeof(TrailMaterialData) + 255) & ~255;
	D3D12_GPU_VIRTUAL_ADDRESS materialBaseAddr = trailBatch_.materialResource->GetGPUVirtualAddress();
	uint8_t* mappedBasePtr = reinterpret_cast<uint8_t*>(trailBatch_.mappedMaterial);

	for (size_t i = 0; i < trailBatches_.size(); ++i)
	{
		const auto& batch = trailBatches_[i];
		if (batch.vertexCount == 0) continue;

		const uint32_t offset = static_cast<uint32_t>(i) * alignedSize;

		// マテリアル更新
		memcpy(mappedBasePtr + offset, &batch.materialData, sizeof(TrailMaterialData));
		cmdList->SetGraphicsRootConstantBufferView(1, materialBaseAddr + offset);

		// テクスチャ・ディゾルブ設定
		cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(batch.textureHandle));
		uint32_t maskHandle = batch.materialData.isDissolveEnabled ? batch.dissolveHandle : batch.textureHandle;
		cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(maskHandle));

		cmdList->DrawInstanced(batch.vertexCount, 1, batch.startVertexIndex, 0);
	}
}

void Renderer::DrawSkybox(const ModelSubmission& sub)
{
	auto* cmdList = commandManager_->GetCommandList();

	// パイプライン設定
	cmdList->SetPipelineState(psoManager_->GetPSO("Skybox"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skybox"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// バッファ設定
	cmdList->IASetVertexBuffers(0, 1, &skyboxMesh_.GetVertexBufferView());
	cmdList->IASetIndexBuffer(&skyboxMesh_.GetIndexBufferView());

	// 定数バッファ・SRV設定
	cmdList->SetGraphicsRootConstantBufferView(0, skyboxMaterialHandle_.resource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, skyboxWvpResource_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));

	// 描画実行
	cmdList->DrawIndexedInstanced(static_cast<UINT>(skyboxMesh_.GetIndexCount()), 1, 0, 0, 0);
}