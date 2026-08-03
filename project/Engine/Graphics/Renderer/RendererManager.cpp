#include "pch.h"
#include "RendererManager.h"
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
#include "ModelRenderer.h"
#include "SpriteRenderer.h"
#include "LineRenderer.h"
#include "ParticleRenderer.h"
#include "TrailRenderer.h"
#include "SkyboxRenderer.h"
#include "GrassRenderer.h"
#include "SkydomeRenderer.h"
#include "TerrainRenderer.h"
#include "TerrainChunk.h"
#include "LightningRenderer.h"

namespace FE
{

RendererManager::RendererManager() {}
RendererManager::~RendererManager() {}

void RendererManager::Initialize(
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
	particleRenderer_ = std::make_unique<ParticleRenderer>();
	particleRenderer_->Initialize(env_);
	trailRenderer_ = std::make_unique<TrailRenderer>();
	trailRenderer_->Initialize(env_);
	skyboxRenderer_ = std::make_unique<SkyboxRenderer>();
	skyboxRenderer_->Initialize(env_);
	grassRenderer_ = std::make_unique<GrassRenderer>();
	skydomeRenderer_ = std::make_unique<SkydomeRenderer>();
	skydomeRenderer_->Initialize(env_);
	terrainRenderer_ = std::make_unique<TerrainRenderer>();
	terrainRenderer_->Initialize(env_);
	lightningRenderer_ = std::make_unique<LightningRenderer>();
	lightningRenderer_->Initialize(env_);

	viewMatrix_ = Matrix4x4::MakeIdentity();
	projectionMatrix_ = Matrix4x4::MakeIdentity();
	viewProjectionMatrix_ = Matrix4x4::MakeIdentity();

	shadowMap_ = shadowMap;
}

void RendererManager::Finalize()
{
	if (modelRenderer_) { modelRenderer_->Finalize(); }
	if (terrainRenderer_) { terrainRenderer_->Finalize(); }
}

void RendererManager::BeginFrame()
{
	if (modelRenderer_) { modelRenderer_->BeginFrame(); }
	if (spriteRenderer_) { spriteRenderer_->BeginFrame(); }
	if (lineRenderer_) { lineRenderer_->BeginFrame(); }
	if (particleRenderer_) { particleRenderer_->BeginFrame(); }
	if (trailRenderer_) { trailRenderer_->BeginFrame(); }
	if (skyboxRenderer_) { skyboxRenderer_->BeginFrame(); }
	if (grassRenderer_) { grassRenderer_->BeginFrame(); }
	if (skydomeRenderer_) { skydomeRenderer_->BeginFrame(); }
	if (terrainRenderer_) { terrainRenderer_->BeginFrame(); }
	if (lightningRenderer_) { lightningRenderer_->BeginFrame(); }
}

void RendererManager::SetCameraState(const Matrix4x4& view, const Matrix4x4& projection, const Vector3& cameraPosition)
{
	viewMatrix_ = view;
	projectionMatrix_ = projection;
	viewProjectionMatrix_ = view * projection;
	cameraPosition_ = cameraPosition;

	if (modelRenderer_)
	{
		modelRenderer_->SetCameraState(viewMatrix_, viewProjectionMatrix_);
	}

	if (terrainRenderer_)
	{
		terrainRenderer_->SetCameraState(viewMatrix_, viewProjectionMatrix_);
	}
}

void RendererManager::DrawFullScreenQuadWithOffscreenTexture()
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
void RendererManager::DrawFinalResult(uint32_t srvIndex)
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

void RendererManager::DrawSceneForShadow(uint32_t cascadeIndex)
{
	if (modelRenderer_)
	{
		modelRenderer_->PrepareBatches();
	}

	auto* cmdList = commandManager_->GetCommandList();
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	if (terrainRenderer_)
	{
		terrainRenderer_->PrepareBatches();
		terrainRenderer_->DrawShadow(env_, cascadeIndex);
	}

	modelRenderer_->DrawShadow(env_, cascadeIndex);
}

void RendererManager::Draw3D()
{
	auto* cmdList = commandManager_->GetCommandList();

	// 共通設定
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 不透明モデルをまとめて描画
	// 不透明オブジェクトの最序盤にTerrainを描画 (Early-Z最適化)
	if (terrainRenderer_)
	{
		terrainRenderer_->Draw(env_, RenderGroup::Opaque, shadowMap_);
	}

	if (modelRenderer_)
	{
		modelRenderer_->Draw(env_, RenderGroup::Opaque, isWireFrame_, shadowMap_);
	}


	if (grassRenderer_)
	{
		// 内部で TRIANGLESTRIP に変更して描画
		grassRenderer_->Draw(env_, grassTextureHandle_, shadowMap_, grassMaterialData_, grassCullingData_);

		// 草の描画が終わったら、以降の描画のために TRIANGLELIST に戻す
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	if (modelRenderer_) 
	{
		modelRenderer_->Draw(env_, RenderGroup::Grid, isWireFrame_, shadowMap_);
	}

	if (skyboxRenderer_)
	{
		skyboxRenderer_->Draw(env_, viewMatrix_, projectionMatrix_);
	}

	if (lineRenderer_)
	{
		lineRenderer_->Draw(env_, viewProjectionMatrix_);
	}

	if (skydomeRenderer_)
	{
		skydomeRenderer_->Draw(env_, viewMatrix_, projectionMatrix_);
	}

	// 半透明モデルをまとめて描画
	if (modelRenderer_)
	{
		modelRenderer_->Draw(env_, RenderGroup::Transparent, isWireFrame_, shadowMap_);
	}

	if (particleRenderer_)
	{
		particleRenderer_->Draw(env_);
	}

	if (trailRenderer_) 
	{
		trailRenderer_->Draw(env_, viewProjectionMatrix_);
	}

	if (lightningRenderer_)
	{
		lightningRenderer_->Draw(env_, viewMatrix_, projectionMatrix_, cameraPosition_);
	}
}

void RendererManager::DrawUI()
{
	auto* cmdList = commandManager_->GetCommandList();

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	if (spriteRenderer_)
	{
		spriteRenderer_->Draw(env_);
	}
}

void RendererManager::SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
	const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
	DepthMode depthMode, RenderGroup group, const Vector4& instanceColor)
{
	if (modelRenderer_)
	{
		modelRenderer_->Submit(worldTransform, modelData, materials, blendMode, cullMode, depthMode, group, instanceColor);
	}
}

void RendererManager::SubmitAnimationModel(
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

void RendererManager::SubmitSprite(const Vector2 position, const Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, const WorldTransform& uvTransform,
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

void RendererManager::SubmitLine(const Vector3& start, const Vector3& end, uint32_t color)
{
	if (lineRenderer_) { lineRenderer_->Submit(start, end, color); }
}

void RendererManager::SubmitParticleInstance(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
	BlendMode blendMode, bool isBillboard, float intensity)
{
	if (particleRenderer_)
	{
		particleRenderer_->Submit(worldTransform, color, textureIndex, rotationZ, blendMode, isBillboard, intensity);
	}
}

void RendererManager::SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config,
	float instanceSeed)
{
	if (trailRenderer_) 
	{
		trailRenderer_->Submit(points, config, cameraPosition_, instanceSeed);
	}
}

void RendererManager::SubmitSkybox(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
	if (skyboxRenderer_)
	{
		skyboxRenderer_->Submit(worldTransform, color, cubeTextureSrvIndex);
	}
}

void RendererManager::InitializeGrass()
{
	if (grassRenderer_)
	{
		grassRenderer_->Initialize(env_);
	}
}

void RendererManager::SetGrassRenderingParams(uint32_t windMapHandle, const GrassMaterialData& materialData, const GrassCullingData& cullingData)
{
	grassTextureHandle_ = windMapHandle;
	grassMaterialData_ = materialData;
	grassCullingData_ = cullingData; 
}

void RendererManager::GenerateGrass(const GrassGenerationData& genData, uint32_t heightMapSrvHandle, uint32_t densityMapSrvHandle)
{
	// 両方のレンダラーが存在しているか確認
	if (grassRenderer_ && terrainRenderer_)
	{
		// TerrainRendererから地形設定のアドレスをもらい、GrassRendererに渡す
		D3D12_GPU_VIRTUAL_ADDRESS terrainSettingsAddr = terrainRenderer_->GetSettingsBuffer()->GetGPUVirtualAddress();

		grassRenderer_->GenerateGrass(
			env_,
			genData,
			heightMapSrvHandle,
			densityMapSrvHandle,
			terrainSettingsAddr
		);
	}
}

void RendererManager::SubmitSkydome(const WorldTransform& worldTransform, uint32_t color, uint32_t cloudNoiseSrvIndex, const AtmosphereSkyData& weather)
{
	if (skydomeRenderer_)
	{
		skydomeRenderer_->Submit(worldTransform, color, cloudNoiseSrvIndex, weather);
	}
}

void RendererManager::SubmitTerrain(const WorldTransform& worldTransform, const TerrainChunk* chunk,
	const Vector4& uvTransform,
	const MaterialHandle& material, const Vector4& instanceColor,
	const Terrain::Parameters& params, uint32_t heightMapHandle)
{
	if (terrainRenderer_)
	{
		terrainRenderer_->Submit(worldTransform, chunk, uvTransform, material, instanceColor, params, heightMapHandle);
	}
}

void RendererManager::SpawnLightning(const Vector3& start, const Vector3& end, float duration)
{
	if (lightningRenderer_)
	{
		lightningRenderer_->SpawnLightning(start, end, duration);
	}
}

void RendererManager::UpdateLightnings()
{
	if (lightningRenderer_)
	{
		lightningRenderer_->Update();
	}
}

void RendererManager::SetLightningConfig(const LightningConfig& config)
{
	if (lightningRenderer_)
	{
		lightningRenderer_->SetConfig(config);
	}
}

uint32_t RendererManager::GetModelCount() const 
{
	return modelRenderer_ ? modelRenderer_->GetCount() : 0;
}
uint32_t RendererManager::GetSpriteCount() const 
{
	return spriteRenderer_ ? spriteRenderer_->GetCount() : 0;
}
uint32_t RendererManager::GetLineCount() const 
{
	return lineRenderer_ ? lineRenderer_->GetCount() : 0;
}
uint32_t RendererManager::GetParticleCount() const
{ 
	return particleRenderer_ ? particleRenderer_->GetCount() : 0;
}
uint32_t RendererManager::GetTrailCount() const
{
	return trailRenderer_ ? trailRenderer_->GetCount() : 0;
}

uint32_t RendererManager::GetMaxModelCount() const 
{
	return modelRenderer_ ? modelRenderer_->GetMaxCount() : 0;
}
uint32_t RendererManager::GetMaxSpriteCount() const 
{
	return spriteRenderer_ ? spriteRenderer_->GetMaxCount() : 0;
}
uint32_t RendererManager::GetMaxLineCount() const 
{
	return lineRenderer_ ? lineRenderer_->GetMaxCount() : 0;
}
uint32_t RendererManager::GetMaxParticleCount() const
{
	return particleRenderer_ ? particleRenderer_->GetMaxCount() : 0;
}
uint32_t RendererManager::GetMaxTrailCount() const
{
	return trailRenderer_ ? trailRenderer_->GetMaxCount() : 0;
}

}