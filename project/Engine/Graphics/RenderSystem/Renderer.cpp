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
#include "TextureHandle.h"
#include "TimeManager.h"

// 最大数の定義
const int32_t Renderer::kMaxModelCount = 500; // モデルの最大数
const int32_t Renderer::kMaxSpriteCount = 101; // スプライトの最大数
const int32_t Renderer::kMaxLineCount = 4096;
const int32_t Renderer::kMaxLineVertices = kMaxLineCount * 2;
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
	TextureManager* textureManager, SRVManager* srvManager, LightManager* lightManager,
	GlobalConstants* globalConstants, MaterialManager* materialManager,
	PostEffectManager* postEffectManager,
	int clientWidth, int clientHeight, ShadowMap* shadowMap)
{
	// ポインタをメンバ変数に保存
	device_ = device;
	commandManager_ = commandManager;
	psoManager_ = psoManager;
	rootSignatureManager_ = rootSignatureManager;
	textureManager_ = textureManager;
	srvManager_ = srvManager;
	lightManager_ = lightManager;
	globalConstants_ = globalConstants;
	materialManager_ = materialManager;
	clientWidth_ = clientWidth;
	clientHeight_ = clientHeight;
	postEffectManager_ = postEffectManager;

	viewMatrix_ = Matrix4x4::MakeIdentity();
	projectionMatrix_ = Matrix4x4::MakeIdentity();
	viewProjectionMatrix_ = Matrix4x4::MakeIdentity();

	CreateObjects();

	shadowMap_ = shadowMap;
}

void Renderer::Finalize()
{
	meshCache.clear();
}

void Renderer::BeginFrame()
{
	prevModelCount_ = indexModel_;
	prevSpriteCount_ = indexSprite_;
	prevLineCount_ = indexLine_;
	prevParticleCount_ = indexParticle_;
	prevTrailCount_ = indexTrail_;

	// 描画カウンタの初期化
	indexModel_ = 0;
	indexSprite_ = 0;
	indexLine_ = 0;
	indexParticle_ = 0;
	indexInstance_ = 0;
	indexTrail_ = 0;
}

void Renderer::CreateObjects()
{
	CreateModels();
	CreateSprites();
	CreateLineBatch();
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
}

int Renderer::LoadTexture(const std::string& texturePath)
{
	// テクスチャをロード
	DirectX::ScratchImage mipImages = TextureManager::LoadTexture(texturePath);

	// テクスチャをアップロード
	TextureManager::TextureResources texResources = textureManager_->UploadTexture(mipImages);

	// 保存したテクスチャのインデックスを返す
	return texResources.srvIndex;
}

void Renderer::LoadTextureArray(const std::vector<std::string>& texturePaths)
{
	// 複数テクスチャをロード
	std::vector<DirectX::ScratchImage> images = textureManager_->LoadMultipleTextures(texturePaths);

	// Texture2DArray作成＆アップロード
	textureManager_->CreateAndUploadTexture2DArray(images, textureArrayResource_);

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

	uint32_t dissolveMapIndex = TextureHandle::Get(TextureID::noise_01);
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		srvManager_->GetSRVHandleGPU(dissolveMapIndex)
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

	uint32_t dissolveMapIndex = TextureHandle::Get(TextureID::noise_01);
	cmdList->SetGraphicsRootDescriptorTable(
		2,
		srvManager_->GetSRVHandleGPU(dissolveMapIndex)
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

	// 通常モデル用の設定
	cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMap"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMap"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ライト行列をセット
	cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());

	for (const auto& sub : modelSubmissions_)
	{
		if (sub.type != RenderType::Model && sub.type != RenderType::Skinning)
		{
			continue;
		}

		if (sub.group == RenderGroup::Background)
		{
			continue;
		}

		if (sub.group == RenderGroup::UI || sub.group == RenderGroup::Transparent)
		{
			continue;
		}

		// メッシュリストを取得して、正しいインデックスのMesh*を取り出す
		const std::vector<Mesh>& meshes = GetOrCreateModelBatch(*sub.modelData);
		assert(sub.meshIndex < meshes.size());
		const Mesh* mesh = &meshes[sub.meshIndex];

		auto& buffer = perObjectBuffers_[sub.instanceIndex];

		bool isSkinning = (sub.skinCluster != nullptr);

		bool needDissolve = (sub.materialHandle.materialData->enableDissolve != 0) ||
			(sub.materialHandle.materialData->color.w < 1.0f);

		// ディゾルブ・透明処理が必要な場合（重い処理）
		if (needDissolve)
		{
			if (isSkinning)
			{
				// スキニング・ディゾルブ影
				cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMapSkinningDissolve"));
				cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMapSkinningDissolve"));

				cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
				cmdList->SetGraphicsRootConstantBufferView(3, sub.materialHandle.resource->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(sub.dissolveTextureHandle));

				const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
				D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
				cmdList->IASetVertexBuffers(0, 2, vbvs);
			}
			else
			{
				// 通常・ディゾルブ影
				cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMapDissolve"));
				cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMapDissolve"));

				cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.dissolveTextureHandle));

				cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			}
		}
		// 不透明の場合（高速処理）
		else
		{
			if (isSkinning)
			{
				cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMapSkinning"));
				cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMapSkinning"));

				cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));

				const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
				D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(),  influence.influenceBufferView };
				cmdList->IASetVertexBuffers(0, 2, vbvs);
			}
			else
			{
				cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMap"));
				cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMap"));

				cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
				cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			}

			// オブジェクト行列
			cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
		}

		// 描画
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	}
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

	// ライン描画を登録
	if (!lineBatch_.verticesCPU.empty())
	{
		ModelSubmission lineSubmission{};
		lineSubmission.type = RenderType::Line;
		lineSubmission.group = RenderGroup::Opaque;
		lineSubmission.depth = 0.0f;
		modelSubmissions_.push_back(lineSubmission);
		indexLine_ = static_cast<uint32_t>(lineBatch_.verticesCPU.size()) / 2;
	}
	else indexLine_ = 0;

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
			case RenderGroup::UI: return a.layerOrder < b.layerOrder;
			default: return a.depth < b.depth;
			}
		});

	auto* cmdList = commandManager_->GetCommandList();

	// 共通設定
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 登録モデルを描画
	for (const auto& sub : modelSubmissions_)
	{
		// UIグループなら描画せずにスキップ
		if (sub.group == RenderGroup::UI)
		{
			continue;
		}

		switch (sub.type)
		{
		case RenderType::Sprite: DrawSprite(sub); break;
		case RenderType::Grid: DrawGrid(sub); break;
		case RenderType::Particle: DrawParticles(); break;
		case RenderType::Trail: DrawTrails(); break;
		case RenderType::Skybox: DrawSkybox(sub); break;
		case RenderType::Model:
		case RenderType::Skinning: DrawModel(sub); break;
		case RenderType::Line:
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			FlushLines();
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		}
	}
}

void Renderer::DrawUI()
{
	auto* cmdList = commandManager_->GetCommandList();

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// UIのみ描画
	for (const auto& sub : modelSubmissions_)
	{
		// UI以外はスキップ
		if (sub.group != RenderGroup::UI)
		{
			continue;
		}

		switch (sub.type)
		{
		case RenderType::Sprite: DrawSprite(sub); break;
			// Lineなども後でやる
		}
	}

	// 後処理
	modelSubmissions_.clear();
	particleBatches_.clear();
	lineBatch_.verticesCPU.clear();
	trailBatch_.verticesCPU.clear();
	trailBatches_.clear();
	hasParticles_ = false;
	currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

std::string Renderer::GetParticlePSOName(BlendMode mode)
{
	switch (mode)
	{
	case kBlendModeNone:     return "ParticleOpaque";
	case kBlendModeNormal:   return "ParticleAlphaBlend";
	case kBlendModeAdd:      return "ParticleAdditive";
	case kBlendModeSubtract: return "ParticleSubtract";
	case kBlendModeMultiply:  return "ParticleMultiply";
	case kBlendModeScreen:   return "ParticleScreen";
	case kBlendModeExclusion: return "ParticleExclusion";
	default:
		assert(false && "Unknown BlendMode");
		return "ParticleOpaque"; // 不明な場合はOpaque
	}
}

const std::vector<Mesh>& Renderer::GetOrCreateModelBatch(const ModelData& modelData)
{
	// キャッシュを検索
	auto it = meshCache.find(&modelData);
	if (it != meshCache.end())
	{
		return it->second.meshes;
	}

	// 新規作成
	ModelBatch batch;
	batch.meshes.resize(modelData.meshes.size());

	for (size_t i = 0; i < modelData.meshes.size(); ++i)
	{
		// 各パーツ(MeshData)からGPUバッファ(Mesh)を生成
		batch.meshes[i].Initialize(
			device_->GetDevice(),
			modelData.meshes[i].vertices,
			modelData.meshes[i].indices
		);
		batch.meshes[i].SetVertexCount(static_cast<uint32_t>(modelData.meshes[i].vertices.size()));
		batch.meshes[i].SetIndexCount(static_cast<uint32_t>(modelData.meshes[i].indices.size()));
	}

	// キャッシュに保存
	meshCache[&modelData] = std::move(batch);
	return meshCache[&modelData].meshes;
}

void Renderer::CreateModels()
{
	perObjectBuffers_.resize(kMaxModelCount);
	for (auto& buffer : perObjectBuffers_)
	{
		// WVPバッファ作成
		buffer.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		buffer.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.wvpMapped));
	}
}

void Renderer::SubmitModel(const WorldTransform& worldTransform, const ModelData& modelData,
	const std::vector<MaterialHandle>& materials, BlendMode blendMode, CullMode cullMode,
	DepthMode depthMode, RenderGroup group, const Vector4& instanceColor)
{
	// モデルに対応するGPUメッシュリストを取得
	const auto& meshes = GetOrCreateModelBatch(modelData);

	// 再帰的にノードを巡回するラムダ関数
	std::function<void(const Node&, const Matrix4x4&)> Traverse =
		[&](const Node& node, const Matrix4x4& parentMatrix)
		{
			// 現在のノードのワールド行列を計算
			Matrix4x4 currentWorldMatrix = node.localMatrix * parentMatrix;

			// このノードが持つすべてのメッシュを描画登録
			for (unsigned int meshIndex : node.meshIndices)
			{
				assert(indexModel_ < kMaxModelCount);

				// 対象のメッシュデータとGPUメッシュを取得
				const auto& meshPart = modelData.meshes[meshIndex];

				// メッシュインデックスに対応するマテリアルを取り出す
				MaterialHandle actualMaterialHandle;
				if (meshIndex < materials.size())
				{
					actualMaterialHandle = materials[meshIndex];
				}
				else
				{
					// 万が一足りない場合は0番目かデフォルトを使う
					actualMaterialHandle = materials.empty() ? meshPart.materialHandle : materials[0];
				}

				// マテリアルからテクスチャ情報を取得する
				uint32_t actualTextureHandle = actualMaterialHandle.textureHandle;
				if (actualTextureHandle == 0)
				{
					actualTextureHandle = meshPart.textureData.textureHandle;
				}

				auto& buffer = perObjectBuffers_[indexModel_];

				// 行列計算 (ノードの階層を考慮した行列を使う)
				Matrix4x4 wvp = currentWorldMatrix * viewProjectionMatrix_;
				buffer.wvpMapped->WVP = wvp;
				buffer.wvpMapped->World = currentWorldMatrix;
				buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(currentWorldMatrix.Transpose());
				buffer.wvpMapped->WorldColor = instanceColor;

				// 描画キューに登録
				ModelSubmission submission{};
				submission.type = RenderType::Model;
				submission.group = group;
				submission.modelData = &modelData;     // 親のModelData
				submission.meshIndex = meshIndex;      // 何番目のメッシュか
				submission.materialHandle = actualMaterialHandle;
				submission.textureHandle = actualTextureHandle;
				submission.envMapSrvHandle = actualMaterialHandle.envMapHandle;
				submission.toonRampHandle = actualMaterialHandle.toonRampHandle;
				submission.dissolveTextureHandle = actualMaterialHandle.dissolveMapHandle;
				submission.normalMapHandle = actualMaterialHandle.normalMapHandle;
				submission.worldMatrix = currentWorldMatrix;
				// マテリアルデータのポインタが存在し、かつenableOutlineがtrueなら有効
				if (actualMaterialHandle.materialData)
				{
					submission.enableOutline = (actualMaterialHandle.materialData->enableOutline != 0);
				}
				else
				{
					submission.enableOutline = false;
				}
				submission.instanceIndex = indexModel_; // 定数バッファのインデックス
				submission.blendMode = blendMode;
				submission.cullMode = cullMode;
				submission.depthMode = depthMode;

				// アルファ判定
				bool hasAlpha = (Math::ColorVectorToUint32(submission.materialHandle.materialData->color) & 0xFF) < 255;
				bool isBlend = submission.blendMode != BlendMode::kBlendModeNone;

				if (hasAlpha || isBlend)
				{
					submission.group = RenderGroup::Transparent;
					if (submission.blendMode == BlendMode::kBlendModeNone)
					{
						submission.blendMode = BlendMode::kBlendModeNormal;
					}
				}
				else
				{
					submission.group = group;
				}

				// 深度設定
				Matrix4x4 worldView = currentWorldMatrix * viewMatrix_;
				submission.depth = worldView.m[3][2];

				modelSubmissions_.push_back(submission);
				indexModel_++;
			}

			// 子ノードへ
			for (const auto& child : node.children)
			{
				Traverse(child, currentWorldMatrix);
			}
		};

	// ルートノードから探索開始
	Traverse(modelData.rootNode, worldTransform.matWorld_);
}

void Renderer::DrawSkeleton(const Skeleton& skeleton, uint32_t color)
{
	for (const Joint& joint : skeleton.joints)
	{
		for (int32_t childIndex : joint.children)
		{
			// 親の位置（行列の平行移動成分）
			Vector3 parentPos = Vector3(
				joint.skeletonSpaceMatrix.m[3][0],
				joint.skeletonSpaceMatrix.m[3][1],
				joint.skeletonSpaceMatrix.m[3][2]
			);

			// 子の位置
			const Joint& child = skeleton.joints[childIndex];
			Vector3 childPos = Vector3(
				child.skeletonSpaceMatrix.m[3][0],
				child.skeletonSpaceMatrix.m[3][1],
				child.skeletonSpaceMatrix.m[3][2]
			);

			SubmitLine(parentPos, childPos, color);
		}
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
	const auto& modelData = instance.modelData;
	// GPUメッシュ生成済みか確認
	GetOrCreateModelBatch(*modelData);

	for (size_t i = 0; i < modelData->meshes.size(); ++i)
	{
		assert(indexModel_ < kMaxModelCount);

		const auto& meshPart = modelData->meshes[i];
		auto& buffer = perObjectBuffers_[indexModel_];

		// 各パーツのWorld行列はモデル全体のWorldで統一される
		Matrix4x4 world = worldTransform.matWorld_;
		Matrix4x4 wvp = world * viewProjectionMatrix_;
		buffer.wvpMapped->WVP = wvp;
		buffer.wvpMapped->World = world;
		buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());
		buffer.wvpMapped->WorldColor = instanceColor;

		// マテリアル決定
		MaterialHandle actualMaterialHandle;
		if (i < materials.size())
		{
			actualMaterialHandle = materials[i];
		}
		else
		{
			// 万が一足りない場合は0番目かデフォルトを使う
			actualMaterialHandle = materials.empty() ? meshPart.materialHandle : materials[0];
		}

		// マテリアルからテクスチャ情報を取得する
		uint32_t actualTextureHandle = actualMaterialHandle.textureHandle;
		if (actualTextureHandle == 0)
		{
			actualTextureHandle = meshPart.textureData.textureHandle;
		}

		// 描画キュー登録
		ModelSubmission submission{};
		submission.type = RenderType::Skinning;
		submission.group = group;
		submission.modelData = modelData;
		submission.meshIndex = static_cast<uint32_t>(i); // 何番目のメッシュか指定
		submission.materialHandle = actualMaterialHandle;
		submission.textureHandle = actualTextureHandle;
		submission.envMapSrvHandle = actualMaterialHandle.envMapHandle;
		submission.toonRampHandle = actualMaterialHandle.toonRampHandle;
		submission.dissolveTextureHandle = actualMaterialHandle.dissolveMapHandle;
		submission.normalMapHandle = actualMaterialHandle.normalMapHandle;
		submission.worldMatrix = world;
		// マテリアルデータのポインタが存在し、かつenableOutlineがtrueなら有効
		if (actualMaterialHandle.materialData)
		{
			submission.enableOutline = (actualMaterialHandle.materialData->enableOutline != 0);
		}
		else {
			submission.enableOutline = false;
		}
		submission.instanceIndex = indexModel_;
		submission.skinCluster = &skinCluster;
		submission.blendMode = blendMode;

		// アルファ判定
		bool hasAlpha = (Math::ColorVectorToUint32(submission.materialHandle.materialData->color) & 0xFF) < 255;
		bool isBlend = submission.blendMode != BlendMode::kBlendModeNone;

		if (hasAlpha || isBlend)
		{
			submission.group = RenderGroup::Transparent;
			if (submission.blendMode == BlendMode::kBlendModeNone)
			{
				submission.blendMode = BlendMode::kBlendModeNormal;
			}
		}
		else
		{
			submission.group = group;
		}

		// 深度設定
		Matrix4x4 worldView = world * viewMatrix_;
		submission.depth = worldView.m[3][2];

		modelSubmissions_.push_back(submission);
		indexModel_++;
	}
}

void Renderer::SubmitGrid(const WorldTransform& worldTransform, const ModelData& modelData, uint32_t textureHandle, uint32_t color, const MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount);

	auto& buffer = perObjectBuffers_[indexModel_];

	// マテリアル設定
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// 行列計算と定数バッファ転送
	Matrix4x4 world = worldTransform.matWorld_;
	Matrix4x4 wvp = world * viewProjectionMatrix_;
	buffer.wvpMapped->WVP = wvp;
	buffer.wvpMapped->World = world;
	buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Grid;
	submission.group = RenderGroup::Grid;
	submission.modelData = &modelData;
	submission.meshIndex = 0;
	submission.materialHandle = materialHandle;
	submission.textureHandle = textureHandle;
	submission.color = color;
	submission.worldMatrix = world;
	submission.instanceIndex = indexModel_;
	submission.skinCluster = nullptr;
	submission.enableOutline = false;

	// 深度設定
	Matrix4x4 worldView = worldTransform.matWorld_ * viewMatrix_;
	submission.depth = worldView.m[3][2];

	modelSubmissions_.push_back(submission);

	indexModel_++;
}

void Renderer::CreateSprites()
{
	// スプライトの配列を確保
	sprites_.resize(kMaxSpriteCount);

	// 左上原点のスプライト用頂点データ
	std::vector<VertexData> spriteVertices =
	{
		{{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 左上
		{{1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 右上
		{{0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下
		{{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 右下
	};

	// スプライト共通のインデックス
	std::vector<uint32_t> spriteIndices = { 0, 1, 2, 1, 3, 2 };

	// スプライト用メッシュとバッファを生成
	for (size_t i = 0; i < kMaxSpriteCount; ++i)
	{
		sprites_[i].mesh.Initialize(device_->GetDevice(), spriteVertices, spriteIndices);

		sprites_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		sprites_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&sprites_[i].mappedData));

		sprites_[i].mesh.SetIndexCount(spriteIndices.size());
	}

	// 使用開始位置をリセット
	indexSprite_ = 0;
}

void Renderer::SubmitSprite(const Vector2 position, const Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, const WorldTransform& uvTransform,
	uint32_t textureHandle, uint32_t dissolveTextureHandle, int layerOrder,
	const MaterialHandle& materialHandle)
{
	assert(indexSprite_ < kMaxSpriteCount);

	RenderData& sprite = sprites_[indexSprite_];

	// マテリアル設定
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);
	materialHandle.materialData->enableLighting = false;

	// UV変換行列設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	materialHandle.materialData->uvTransform = uvTransformMatrix;

	// 行列計算

	// アンカーポイント分ずらす行列
	Matrix4x4 anchorMatrix = Matrix4x4::MakeTranslate({ -anchorPoint.x, -anchorPoint.y, 0.0f });

	// スケーリング行列
	Matrix4x4 scaleMatrix = Matrix4x4::MakeScale({ size.x, size.y, 1.0f });

	// 回転行列
	Matrix4x4 rotationMatrix = Matrix4x4::MakeRotateZ(rotation);

	// 平行移動行列
	Matrix4x4 translateMatrix = Matrix4x4::MakeTranslate({ position.x, position.y, 0.0f });

	// 全て合成してワールド行列を作る
	sprite.worldMatrix = anchorMatrix * scaleMatrix * rotationMatrix * translateMatrix;

	// 平行投影行列を作成
	Matrix4x4 projectionMatrix = Matrix4x4::MakeOrthographic(
		0.0f, 0.0f, float(clientWidth_), float(clientHeight_),
		0.0f, 100.0f
	);

	Matrix4x4 wvpMatrix = sprite.worldMatrix * projectionMatrix;

	// 定数バッファにコピー
	sprite.mappedData->WVP = wvpMatrix;
	sprite.mappedData->World = sprite.worldMatrix;

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Sprite;
	submission.group = RenderGroup::UI;
	submission.instanceIndex = indexSprite_;
	submission.textureHandle = textureHandle;
	submission.dissolveTextureHandle = dissolveTextureHandle;
	submission.materialHandle = materialHandle;
	submission.color = color;
	submission.worldMatrix = sprite.worldMatrix;
	submission.depth = 0.0f;
	submission.layerOrder = layerOrder;

	modelSubmissions_.push_back(submission);

	indexSprite_++;
}

void Renderer::CreateLineBatch()
{
	// 動的頂点バッファ作成
	lineBatch_.mesh.CreateDynamicMesh(device_->GetDevice(), kMaxLineVertices, sizeof(LineVertex));

	// CPU側配列を予約
	lineBatch_.verticesCPU.reserve(kMaxLineVertices);

	// WVP用定数バッファ作成
	lineBatch_.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
	lineBatch_.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&lineBatch_.mappedWvp));
	lineBatch_.mappedWvp->WVP = Matrix4x4::MakeIdentity();
}

void Renderer::SubmitLine(const Vector3& start, const Vector3& end, uint32_t color)
{
	if (lineBatch_.verticesCPU.size() >= kMaxLineVertices) return;

	Vector4 colorVec = Math::Uint32ToColorVector(color);

	// 頂点作成
	LineVertex v1{ {start.x, start.y, start.z, 1.0f}, colorVec };
	LineVertex v2{ {end.x, end.y, end.z, 1.0f}, colorVec };

	// CPUバッファに追加
	lineBatch_.verticesCPU.push_back(v1);
	lineBatch_.verticesCPU.push_back(v2);
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
	uint32_t textureHandle = ParticleTextureHandle::Get(config.textureID);
	uint32_t dissolveHandle =
		(config.dissolveTextureID >= 0)
		? ParticleTextureHandle::Get(static_cast<ParticleTextureID>(config.dissolveTextureID))
		: 0;

	// マテリアル定数
	TrailMaterialData currentMatData{};
	currentMatData.scrollSpeed = config.scrollSpeed;
	currentMatData.jitterStrength = config.jitterStrength;
	currentMatData.jitterFrequency = config.jitterFrequency;
	currentMatData.jitterSpeed = config.jitterSpeed;
	currentMatData.jitterMode = static_cast<int>(config.jitterMode);
	currentMatData.jitterPhase = config.jitterPhase;
	currentMatData.isDissolveEnabled = (config.dissolveTextureID >= 0) ? 1 : 0;
	currentMatData.emissiveIntensity = config.emissiveIntensity;

	// バッチ切り替え判定
	bool isNewBatch = trailBatches_.empty();
	if (!isNewBatch)
	{
		const auto& last = trailBatches_.back();
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

void Renderer::DrawSprite(const ModelSubmission& sub)
{
	RenderData& sprite = sprites_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetPipelineState(psoManager_->GetPSO("Sprite"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Sprite"));

	cmdList->IASetIndexBuffer(&sprite.mesh.GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &sprite.mesh.GetVertexBufferView());

	cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, sprite.wvpResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));
	cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.dissolveTextureHandle));

	cmdList->DrawIndexedInstanced(UINT(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
}

void Renderer::DrawModel(const ModelSubmission& sub)
{
	// モデルデータに対応するメッシュリストを取得
	const std::vector<Mesh>& meshes = GetOrCreateModelBatch(*sub.modelData);

	// 今回の描画コマンドで指定されたインデックスのメッシュを取得
	assert(sub.meshIndex < meshes.size());
	const Mesh* mesh = &meshes[sub.meshIndex];

	auto& buffer = perObjectBuffers_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bool isSkinning = (sub.skinCluster != nullptr);

	std::string psoName;

	// スキニングかどうかで分岐
	if (isSkinning)
	{
		psoName = "Skinning";
	}
	else if (isWireFrame_)
	{
		psoName = "Wireframe";
	}
	else
	{
		// 通常モデル (ブレンドモードで分岐)
		switch (sub.blendMode)
		{
		case BlendMode::kBlendModeAdd:      psoName = "Object3DAdd";         break;
		case BlendMode::kBlendModeNormal:   psoName = "Object3DTransparent"; break;
		case BlendMode::kBlendModeNone:
		default:                            psoName = "Standard3D";          break;
		}
	}

	// ワイヤーフレーム以外の場合、カリングとデプスの設定を名前に付与する
	if (psoName != "Wireframe")
	{
		// カリング設定の接尾辞追加
		if (sub.cullMode == CullMode::None)
		{
			psoName += "_NoCull";
		}
		else if (sub.cullMode == CullMode::Front)
		{
			psoName += "_FrontCull";
		}

		// デプス設定の接尾辞追加
		if (sub.depthMode == DepthMode::ReadOnly)
		{
			psoName += "_DepthRead";
		}
		else if (sub.depthMode == DepthMode::None)
		{
			psoName += "_DepthOff";
		}
	}

	// アウトライン描画
	if (sub.enableOutline)
	{
		if (isSkinning)
		{
			cmdList->SetPipelineState(psoManager_->GetPSO("SkinningOutline"));
			cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("SkinningOutline"));

			cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
			cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(3, globalConstants_->GetResource()->GetGPUVirtualAddress());

			const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
			D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
			cmdList->IASetVertexBuffers(0, 2, vbvs);
		}
		else
		{
			cmdList->SetPipelineState(psoManager_->GetPSO("Object3DOutline"));
			cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Outline"));

			cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(1, sub.materialHandle.resource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(2, globalConstants_->GetResource()->GetGPUVirtualAddress());
		}

		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	}

	// スキニング描画
	if (isSkinning)
	{
		// 生成した名前でPSOを検索
		ID3D12PipelineState* pso = psoManager_->GetPSO(psoName);

		// もしその組み合わせのPSOを作っていなかった場合の安全策 (フォールバック)
		if (!pso)
		{
			pso = psoManager_->GetPSO("Skinning"); // 基本に戻す
		}
		cmdList->SetPipelineState(pso);
		cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skinning"));

		const auto& influence = sub.skinCluster->meshInfluences[sub.meshIndex];
		D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), influence.influenceBufferView };
		cmdList->IASetVertexBuffers(0, 2, vbvs);
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
		cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.textureHandle));
		cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(sub.envMapSrvHandle));
		cmdList->SetGraphicsRootConstantBufferView(5, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(6, globalConstants_->GetResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(9, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(10, shadowMap_->GetSRVHandle());
		cmdList->SetGraphicsRootDescriptorTable(11, srvManager_->GetSRVHandleGPU(sub.toonRampHandle));
		cmdList->SetGraphicsRootDescriptorTable(12, srvManager_->GetSRVHandleGPU(sub.dissolveTextureHandle));
		cmdList->SetGraphicsRootDescriptorTable(13, srvManager_->GetSRVHandleGPU(sub.normalMapHandle));
	}

	// 通常モデル描画
	else
	{
		// 通常モデル
		ID3D12PipelineState* pso = psoManager_->GetPSO(psoName);

		// 安全策
		if (!pso)
		{
			// 見つからなければ標準的なものを使用
			pso = psoManager_->GetPSO("Standard3D");
		}

		cmdList->SetPipelineState(pso);
		cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));

		cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));
		cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.envMapSrvHandle));
		cmdList->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(5, globalConstants_->GetResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(9, shadowMap_->GetSRVHandle());
		cmdList->SetGraphicsRootDescriptorTable(10, srvManager_->GetSRVHandleGPU(sub.toonRampHandle));
		cmdList->SetGraphicsRootDescriptorTable(11, srvManager_->GetSRVHandleGPU(sub.dissolveTextureHandle));
		cmdList->SetGraphicsRootDescriptorTable(12, srvManager_->GetSRVHandleGPU(sub.normalMapHandle));
	}

	cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
}

void Renderer::DrawGrid(const ModelSubmission& sub)
{
	const std::vector<Mesh>& meshes = GetOrCreateModelBatch(*sub.modelData);

	// meshIndexを使って描画対象のメッシュを特定
	if (sub.meshIndex >= meshes.size()) {
		return;
	}
	const Mesh* mesh = &meshes[sub.meshIndex];

	auto& buffer = perObjectBuffers_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();

	// パイプライン・トポロジー設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetPipelineState(psoManager_->GetPSO("Grid"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));

	// バッファ設定
	cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());

	// 基本リソース設定
	cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));
	cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.textureHandle));

	// ライト・カメラ情報設定
	cmdList->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(5, globalConstants_->GetResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// シャドウマップ・追加リソース設定
	cmdList->SetGraphicsRootDescriptorTable(9, shadowMap_->GetSRVHandle());
	cmdList->SetGraphicsRootDescriptorTable(10, srvManager_->GetSRVHandleGPU(0));

	// 描画実行
	cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
}

void Renderer::FlushLines()
{
	// 線がなければ終了
	if (lineBatch_.verticesCPU.empty()) return;

	// カメラ行列更新
	lineBatch_.mappedWvp->WVP = viewProjectionMatrix_;

	// CPUデータをGPUバッファにコピー
	LineVertex* gpuPtr = nullptr;
	lineBatch_.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&gpuPtr));
	std::memcpy(gpuPtr, lineBatch_.verticesCPU.data(), sizeof(LineVertex) * lineBatch_.verticesCPU.size());
	lineBatch_.mesh.GetVertexResource()->Unmap(0, nullptr);

	// 描画コマンド発行
	auto* cmdList = commandManager_->GetCommandList();
	cmdList->SetPipelineState(psoManager_->GetPSO("Line"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Line"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// 頂点バッファセット
	D3D12_VERTEX_BUFFER_VIEW vbView = lineBatch_.mesh.GetVertexBufferView();
	cmdList->IASetVertexBuffers(0, 1, &vbView);

	// 定数バッファ(WVP)セット
	cmdList->SetGraphicsRootConstantBufferView(0, lineBatch_.wvpResource->GetGPUVirtualAddress());

	// 描画
	cmdList->DrawInstanced(static_cast<UINT>(lineBatch_.verticesCPU.size()), 1, 0, 0);
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