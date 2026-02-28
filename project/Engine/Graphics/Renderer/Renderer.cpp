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
#include "ModelRenderer.h"
#include "SpriteRenderer.h"
#include "LineRenderer.h"
#include "ParticleRenderer.h"
#include "TrailRenderer.h"
#include "SkyboxRenderer.h"

Renderer::Renderer() {}
Renderer::~Renderer() {}

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
	particleRenderer_ = std::make_unique<ParticleRenderer>();
	particleRenderer_->Initialize(env_);
	trailRenderer_ = std::make_unique<TrailRenderer>();
	trailRenderer_->Initialize(env_);
	skyboxRenderer_ = std::make_unique<SkyboxRenderer>();
	skyboxRenderer_->Initialize(env_);

	viewMatrix_ = Matrix4x4::MakeIdentity();
	projectionMatrix_ = Matrix4x4::MakeIdentity();
	viewProjectionMatrix_ = Matrix4x4::MakeIdentity();

	shadowMap_ = shadowMap;
}

void Renderer::Finalize()
{
	modelRenderer_->Finalize();
}

void Renderer::BeginFrame()
{
	if (modelRenderer_) { modelRenderer_->BeginFrame(); }
	if (spriteRenderer_) { spriteRenderer_->BeginFrame(); }
	if (lineRenderer_) { lineRenderer_->BeginFrame(); }
	if (particleRenderer_) { particleRenderer_->BeginFrame(); }
	if (trailRenderer_) { trailRenderer_->BeginFrame(); }
	if (skyboxRenderer_) { skyboxRenderer_->BeginFrame(); }
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

	if (skyboxRenderer_)
	{
		skyboxRenderer_->Draw(env_, viewMatrix_, projectionMatrix_);
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

	if (particleRenderer_)
	{
		particleRenderer_->Draw(env_);
	}

	if (trailRenderer_) 
	{
		trailRenderer_->Draw(env_, viewProjectionMatrix_);
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

void Renderer::SubmitParticleInstance(const WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
	BlendMode blendMode, bool isBillboard, float intensity)
{
	if (particleRenderer_)
	{
		particleRenderer_->Submit(worldTransform, color, textureIndex, rotationZ, blendMode, isBillboard, intensity);
	}
}

void Renderer::SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config)
{
	if (trailRenderer_) 
	{
		trailRenderer_->Submit(points, config, cameraPosition_);
	}
}

void Renderer::SubmitSkybox(const WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
	if (skyboxRenderer_)
	{
		skyboxRenderer_->Submit(worldTransform, color, cubeTextureSrvIndex);
	}
}

uint32_t Renderer::GetModelCount() const 
{
	return modelRenderer_ ? modelRenderer_->GetCount() : 0;
}
uint32_t Renderer::GetSpriteCount() const 
{
	return spriteRenderer_ ? spriteRenderer_->GetCount() : 0;
}
uint32_t Renderer::GetLineCount() const 
{
	return lineRenderer_ ? lineRenderer_->GetCount() : 0;
}
uint32_t Renderer::GetParticleCount() const
{ 
	return particleRenderer_ ? particleRenderer_->GetCount() : 0;
}
uint32_t Renderer::GetTrailCount() const
{
	return trailRenderer_ ? trailRenderer_->GetCount() : 0;
}

uint32_t Renderer::GetMaxModelCount() const 
{
	return modelRenderer_ ? modelRenderer_->GetMaxCount() : 0;
}
uint32_t Renderer::GetMaxSpriteCount() const 
{
	return spriteRenderer_ ? spriteRenderer_->GetMaxCount() : 0;
}
uint32_t Renderer::GetMaxLineCount() const 
{
	return lineRenderer_ ? lineRenderer_->GetMaxCount() : 0;
}
uint32_t Renderer::GetMaxParticleCount() const
{
	return particleRenderer_ ? particleRenderer_->GetMaxCount() : 0;
}
uint32_t Renderer::GetMaxTrailCount() const
{
	return trailRenderer_ ? trailRenderer_->GetMaxCount() : 0;
}