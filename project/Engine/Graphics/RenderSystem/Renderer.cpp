#include "Renderer.h"
#include "GraphicsDevice.h"
#include "CommandManager.h"
#include "PSOManager.h"
#include "RootSignatureManager.h"
#include "LightManager.h"
#include "SRVManager.h"
#include "CameraManager.h"
#include "MaterialManager.h"
#include "BufferManager.h"
#include "ShapeGenerator.h"
#include "Camera.h"
#include "PostEffectManager.h"
#include "TimeManager.h"

// 最大数の定義
const int32_t Renderer::kMaxTriangleCount = 0; // 三角形の最大数
const int32_t Renderer::kMaxSphereCount = 0; // 球の最大数
const int32_t Renderer::kMaxModelCount = 500; // モデルの最大数
const int32_t Renderer::kMaxSpriteCount = 101; // スプライトの最大数
const int32_t Renderer::kMaxCubeCount = 0;// 立方体の最大数
const int32_t Renderer::kMaxLineCount = 200;// ラインの最大数
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
	CameraManager* cameraManager, MaterialManager* materialManager, Camera* camera,
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
	cameraManager_ = cameraManager;
	materialManager_ = materialManager;
	camera_ = camera;
	clientWidth_ = clientWidth;
	clientHeight_ = clientHeight;
	postEffectManager_ = postEffectManager;

	CreateObjects();

	shadowMap_ = shadowMap;
}

void Renderer::Finalize()
{
	meshCache.clear();
}

void Renderer::BeginFrame()
{
	// 描画カウンタの初期化
	indexSphere_ = 0;
	indexModel_ = 0;
	indexSprite_ = 0;
	indexTriangle_ = 0;
	indexCube_ = 0;
	indexLine_ = 0;
	indexParticle_ = 0;
	indexInstance_ = 0;
	indexTrail_ = 0;

	if (frameData_)
	{
		frameData_->gTime += static_cast<float>(TimeManager::GetInstance()->GetTotalTime());

		// 非常に大きな値になるのを防ぐ
		if (frameData_->gTime > 10000.0f)
		{
			frameData_->gTime = 0.0f;
		}
		frameData_->iResolution = Vector2(1, 1);
		frameData_->screenResolution = Vector2(static_cast<float>(clientWidth_), static_cast<float>(clientHeight_));
	}

}

void Renderer::CreateObjects()
{
	CreateSpheres();
	CreateModels();
	CreateSprites();
	CreateTriangles();
	CreateCubes();
	CreateLines();
	CreateParticles();
	CreateSkybox();
	CreateTrails();
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
	// コマンドリストのローカル変数を取得
	auto* cmdList = commandManager_->GetCommandList();

	// SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	// パイプラインステートをセット（フルスクリーン描画用PSO）
	cmdList->SetPipelineState(psoManager_->GetPSO("Fullscreen"));

	// ルートシグネチャをセット（フルスクリーン用のルートシグネチャ）
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Fullscreen"));

	// ルートパラメータにSRVなどをセット
	cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(postEffectManager_->bloomCombineIndex_));
	cmdList->SetGraphicsRootConstantBufferView(0, postEffectManager_->constantBuffer_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(postEffectManager_->depthExtractIndex_));

	// プリミティブトポロジーを設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファなし

	// DrawCall（3頂点の三角形）
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
		if (sub.type != RenderType::Model && sub.type != RenderType::Skinning) {
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

		Mesh* mesh = GetOrCreateMesh(*sub.modelData);
		auto& buffer = perObjectBuffers_[sub.instanceIndex];

		bool isSkinning = (sub.skinCluster != nullptr);

		if (isSkinning)
		{
			// スキニング用の設定
			cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMapSkinning"));
			cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMapSkinning"));
			cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));

			D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), sub.skinCluster->influenceBufferView };
			cmdList->IASetVertexBuffers(0, 2, vbvs);
		}
		else
		{
			// 通常モデル用の設定
			cmdList->SetPipelineState(psoManager_->GetPSO("ShadowMap"));
			cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("ShadowMap"));
			cmdList->SetGraphicsRootConstantBufferView(1, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
			cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
		}

		// オブジェクト行列をセット
		cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());

		// 描画
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	}
}
void Renderer::DrawScene()
{
	if (hasParticles_)
	{
		ModelSubmission particleSubmission{};
		particleSubmission.type = RenderType::Particle;

		// 半透明グループに所属させる
		particleSubmission.group = RenderGroup::Particle;

		// 深度設定: 
		particleSubmission.depth = 0.0f;

		modelSubmissions_.push_back(particleSubmission);
	}

	// 描画順にソート（グループ→深度→UI順）
	std::sort(modelSubmissions_.begin(), modelSubmissions_.end(),
		[](const ModelSubmission& a, const ModelSubmission& b)
		{
			if (a.group != b.group)
			{
				return a.group < b.group;
			}
			switch (a.group)
			{
			case RenderGroup::Opaque:      return a.depth < b.depth;
			case RenderGroup::Grid:         return a.depth < b.depth;
			case RenderGroup::Transparent: return a.depth > b.depth;
			case RenderGroup::UI:          return a.layerOrder < b.layerOrder;
			default:                       return a.depth < b.depth;
			}
		});

	auto* cmdList = commandManager_->GetCommandList();

	// 共通設定（SRVヒープ、プリミティブタイプ）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 登録済みモデルを描画
	for (const auto& sub : modelSubmissions_)
	{
		switch (sub.type)
		{
		case RenderType::Sprite:
			DrawSprite(sub);
			break;
		case RenderType::Grid:
			DrawGrid(sub);
			break;
		case RenderType::Line:
			DrawLine(sub);
			break;
		case RenderType::Particle:
			DrawParticles(*camera_);
			break;
		case RenderType::Trail:
			DrawTrail(sub);
			break;
		case RenderType::Skybox:
			DrawSkybox(sub);
			break;
		case RenderType::Model:
		case RenderType::Skinning:
			DrawModel(sub);
			break;
		}
	}

	modelSubmissions_.clear();

	particleBatches_.clear();
	hasParticles_ = false;
	currentFrameIndex_ = (currentFrameIndex_ + 1) % kFrameCount;
}

Matrix4x4 Renderer::MakeCenteredAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate, Vector3 pivot)
{
	Matrix4x4 moveToOrigin = Matrix4x4::MakeTranslate({ -pivot.x, -pivot.y, -pivot.z });
	Matrix4x4 rotateScale = Matrix4x4::MakeAffine(scale, rotate, { 0.0f, 0.0f, 0.0f });
	Matrix4x4 moveBack = Matrix4x4::MakeTranslate(pivot);
	Matrix4x4 result = (moveToOrigin * rotateScale) * moveBack;
	return result * Matrix4x4::MakeTranslate(translate);
}

std::string Renderer::GetParticlePSOName(BlendMode mode)
{
	switch (mode)
	{
	case kBlendModeNone:     return "ParticleOpaque";
	case kBlendModeNormal:   return "ParticleAlphaBlend";
	case kBlendModeAdd:      return "ParticleAdditive";
	case kBlendModeSubtract: return "ParticleSubtract";
	case kBlendModeMultily:  return "ParticleMultiply";
	case kBlendModeScreen:   return "ParticleScreen";
	case kBlendModeExclusion: return "ParticleExclusion";
	default:
		assert(false && "Unknown BlendMode");
		return "ParticleOpaque"; // 不明な場合はOpaque
	}
}

void Renderer::CreateTriangles()
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
		triangles_[i].mesh.InitializeVertexOnly(device_->GetDevice(), triangleVertices);
		// マテリアルを作成・設定
		triangles_[i].materialHandle = materialManager_->CreateMaterial(device_->GetDevice());

		triangles_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();// UV行列は単位行列で初期化

		// WVP行列用のバッファを作成
		triangles_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		triangles_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&triangles_[i].mappedData));  // CPUアクセス用にマッピング

		triangles_[i].mesh.SetVertexCount(triangleVertices.size()); // インデックス数を設定
	}

	// 最初に使用するスフィアのインデックスをリセット
	indexTriangle_ = 0;
}

void Renderer::DrawTriangle(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t envMapSrvHandle)
{
	assert(indexTriangle_ < kMaxTriangleCount); // 配列範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& triangle = triangles_[indexTriangle_];

	// マテリアル色を設定
	triangle.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列を中心を基準に計算
	Vector3 pivot = { 320.0f, 180.0f, 0.0f }; // 三角形の中心
	triangle.worldMatrix = MakeCenteredAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_, pivot);

	// WVP行列（World * Orthographic）を計算してGPUバッファにコピー
	Matrix4x4 wvpMatrix = triangle.worldMatrix * Matrix4x4::MakeOrthographic(0, 0, float(clientWidth_), float(clientHeight_), 0, 100);
	memcpy(&triangle.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	triangle.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(triangle.worldMatrix.Transpose());

	// UV変換行列をマテリアルに設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	triangle.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプライン・ルートシグネチャ・プリミティブ設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Standard3D"));
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &triangle.mesh.GetVertexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, triangle.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, triangle.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle)); // テクスチャ
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(envMapSrvHandle)); // 環境マップ
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(9, srvManager_->GetSRVHandleGPU(textureHandle));

	// 描画
	commandManager_->GetCommandList()->DrawInstanced(UINT(triangle.mesh.GetVertexCount()), 1, 0, 0);

	indexTriangle_++; // 使用カウント更新
}

void Renderer::CreateSpheres()
{
	spheres_.resize(kMaxSphereCount);

	std::vector<VertexData> sphereVertices;
	std::vector<uint32_t> sphereIndices;

	ShapeGenerator shapeGenerator;
	shapeGenerator.SphereGenerator(sphereVertices, sphereIndices);

	for (size_t i = 0; i < kMaxSphereCount; ++i)
	{
		spheres_[i].mesh.Initialize(device_->GetDevice(), sphereVertices, sphereIndices);

		spheres_[i].materialHandle = materialManager_->CreateMaterial(device_->GetDevice());
		spheres_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();

		spheres_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		spheres_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&spheres_[i].mappedData));

		spheres_[i].mesh.SetIndexCount(sphereIndices.size());
	}
	indexSphere_ = 0;
}

void Renderer::DrawSphere(WorldTransform& worldTransform, Camera& camera, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t envMapSrvHandle, uint32_t color)
{
	assert(indexSphere_ < kMaxSphereCount); // 配列範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& sphere = spheres_[indexSphere_];

	// マテリアル色を設定
	sphere.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列を設定
	sphere.worldMatrix = worldTransform.matWorld_;

	// WVP行列（World * ViewProjection）を計算してGPUバッファにコピー
	Matrix4x4 wvpMatrix = sphere.worldMatrix * camera.GetViewProjectionMatrix();
	memcpy(&sphere.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	sphere.mappedData->World = sphere.worldMatrix;
	sphere.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(sphere.worldMatrix.Transpose());

	// UV変換行列を設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	sphere.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプライン・ルートシグネチャ・プリミティブ設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Standard3D"));
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &sphere.mesh.GetVertexBufferView());
	commandManager_->GetCommandList()->IASetIndexBuffer(&sphere.mesh.GetIndexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sphere.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sphere.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle)); // テクスチャ
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(envMapSrvHandle)); // 環境マップ
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(9, srvManager_->GetSRVHandleGPU(textureHandle));


	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sphere.mesh.GetIndexCount()), 1, 0, 0, 0);

	indexSphere_++; // 使用カウント更新
}

Mesh* Renderer::GetOrCreateMesh(const ModelData& modelData)
{
	auto it = meshCache.find(&modelData);
	if (it != meshCache.end())
	{
		return &it->second;
	}

	Mesh newMesh;
	newMesh.Initialize(device_->GetDevice(), modelData.vertices, modelData.indices);
	newMesh.SetVertexCount(static_cast<uint32_t>(modelData.vertices.size()));
	newMesh.SetIndexCount(static_cast<uint32_t>(modelData.indices.size()));

	meshCache[&modelData] = std::move(newMesh);
	return &meshCache[&modelData];
}

void Renderer::CreateModels()
{
	perObjectBuffers_.resize(kMaxModelCount);
	for (auto& buffer : perObjectBuffers_)
	{
		// WVPバッファ作成
		buffer.wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		buffer.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.wvpMapped));

		// Outlineバッファ作成
		buffer.outlineResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(OutlineData));
		buffer.outlineResource->Map(0, nullptr, reinterpret_cast<void**>(&buffer.outlineMapped));
	}
}

void Renderer::SubmitModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData,
	uint32_t textureHandle, uint32_t envMapSrvHandle, uint32_t toonRampHandle, uint32_t color, MaterialHandle& materialHandle, BlendMode blendMode,
	bool enableOutline, float outlineWidth, const Vector4& outlineColor, RenderGroup group)
{
	assert(indexModel_ < kMaxModelCount);

	auto& buffer = perObjectBuffers_[indexModel_];

	// 行列計算と定数バッファ転送
	Matrix4x4 world = worldTransform.matWorld_;
	Matrix4x4 wvp = world * camera.GetViewProjectionMatrix();
	buffer.wvpMapped->WVP = wvp;
	buffer.wvpMapped->World = world;
	buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());

	// アウトライン設定
	if (enableOutline) {
		buffer.outlineMapped->color = outlineColor;
		buffer.outlineMapped->width = outlineWidth;
	}

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Model;
	submission.group = group;
	submission.modelData = &modelData;
	submission.materialHandle = materialHandle;
	submission.textureHandle = textureHandle;
	submission.envMapSrvHandle = envMapSrvHandle;
	submission.toonRampHandle = toonRampHandle;
	submission.color = color;
	submission.worldMatrix = world;
	submission.enableOutline = enableOutline;
	submission.instanceIndex = indexModel_;
	submission.blendMode = blendMode;

	bool hasAlpha = ((color >> 24) & 0xFF) < 255;
	bool isBlend = submission.blendMode != BlendMode::kBlendModeNone;

	if (hasAlpha || isBlend)
	{
		// アルファ成分がある、または加算/半透明モードなら強制的にTransparentグループへ
		submission.group = RenderGroup::Transparent;

		// もしモードが None(不透明) なのにアルファ値があるなら、Normal(半透明)扱いに変更
		if (submission.blendMode == BlendMode::kBlendModeNone)
		{
			submission.blendMode = BlendMode::kBlendModeNormal;
		}
	}
	else
	{
		// それ以外は引数のグループを使う
		submission.group = group;
	}

	// 深度設定
	Matrix4x4 worldView = world * camera.GetViewMatrix();
	submission.depth = worldView.m[3][2];

	modelSubmissions_.push_back(submission);
	indexModel_++;
}

void Renderer::DrawSkeleton(const Skeleton& skeleton, Camera& camera, uint32_t color)
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

			SubmitLine(parentPos, childPos, camera, color);
		}
	}
}

void Renderer::SubmitAnimationModel(
	WorldTransform& worldTransform,
	Camera& camera,
	const AnimatedModelData& instance,
	const SkinCluster& skinCluster,
	uint32_t textureHandle,
	uint32_t envMapSrvHandle,
	uint32_t toonRampHandle,
	uint32_t color,
	MaterialHandle& materialHandle,
	bool enableOutline,
	float outlineWidth,
	const Vector4& outlineColor,
	RenderGroup group)
{
	assert(indexModel_ < kMaxModelCount);

	auto& buffer = perObjectBuffers_[indexModel_];

	// 行列計算と定数バッファ転送
	Matrix4x4 world = worldTransform.matWorld_;
	Matrix4x4 wvp = world * camera.GetViewProjectionMatrix();
	buffer.wvpMapped->WVP = wvp;
	buffer.wvpMapped->World = world;
	buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());

	// アウトライン設定
	if (enableOutline) {
		buffer.outlineMapped->color = outlineColor;
		buffer.outlineMapped->width = outlineWidth;
	}

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Skinning;
	submission.group = group;
	submission.modelData = &instance.modelData;
	submission.materialHandle = materialHandle;
	submission.textureHandle = textureHandle;
	submission.envMapSrvHandle = envMapSrvHandle;
	submission.toonRampHandle = toonRampHandle;
	submission.color = color;
	submission.worldMatrix = world;
	submission.enableOutline = enableOutline;
	submission.instanceIndex = indexModel_;
	submission.skinCluster = &skinCluster;

	// 深度設定
	Matrix4x4 worldView = world * camera.GetViewMatrix();
	submission.depth = worldView.m[3][2];

	modelSubmissions_.push_back(submission);
	indexModel_++;
}

void Renderer::SubmitGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color, MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount);

	auto& buffer = perObjectBuffers_[indexModel_];

	// マテリアル設定
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// 行列計算と定数バッファ転送
	Matrix4x4 world = worldTransform.matWorld_;
	Matrix4x4 wvp = world * camera.GetViewProjectionMatrix();
	buffer.wvpMapped->WVP = wvp;
	buffer.wvpMapped->World = world;
	buffer.wvpMapped->WorldInverseTranspose = Matrix4x4::Inverse(world.Transpose());

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Grid;
	submission.group = RenderGroup::Grid;
	submission.modelData = &modelData;
	submission.materialHandle = materialHandle;
	submission.textureHandle = textureHandle;
	submission.color = color;
	submission.worldMatrix = world;
	submission.instanceIndex = indexModel_;
	submission.skinCluster = nullptr;
	submission.enableOutline = false;

	// 深度設定
	Matrix4x4 worldView = worldTransform.matWorld_ * camera.GetViewMatrix();
	submission.depth = worldView.m[3][2];

	modelSubmissions_.push_back(submission);

	indexModel_++;
}

void Renderer::CreateSprites()
{
	// スプライトの配列を確保
	sprites_.resize(kMaxSpriteCount);

	// 左上原点のスプライト用頂点データ
	std::vector<VertexData> spriteVertices = {
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

void Renderer::SubmitSprite(Vector2 position, Vector2 size, float rotation, uint32_t color, const Vector2& anchorPoint, WorldTransform& uvTransform, uint32_t textureHandle, int layerOrder,
	MaterialHandle& materialHandle)
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
	submission.materialHandle = materialHandle;
	submission.color = color;
	submission.worldMatrix = sprite.worldMatrix;
	submission.depth = 0.0f;
	submission.layerOrder = layerOrder;

	modelSubmissions_.push_back(submission);

	indexSprite_++;
}

void Renderer::CreateCubes()
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
		cubes_[i].mesh.Initialize(device_->GetDevice(), cubeVertices, cubeIndices);

		// マテリアル作成
		cubes_[i].materialHandle = materialManager_->CreateMaterial(device_->GetDevice());
		cubes_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();

		// WVPバッファ作成
		cubes_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		cubes_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&cubes_[i].mappedData));

		cubes_[i].mesh.SetIndexCount(cubeIndices.size());
	}

	indexCube_ = 0;
}

void Renderer::DrawCube(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t envMapSrvHandle)
{
	assert(indexCube_ < kMaxCubeCount); // 配列の範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& cube = cubes_[indexCube_];

	// マテリアル色を設定
	cube.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列を計算
	cube.worldMatrix = Matrix4x4::MakeAffine(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

	// WVP行列を計算してGPUバッファにコピー
	camera_->UpdateViewProjectionMatrix();
	Matrix4x4 wvpMatrix = cube.worldMatrix * camera_->GetViewProjectionMatrix();
	memcpy(&cube.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));
	cube.mappedData->World = cube.worldMatrix;
	cube.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(cube.worldMatrix.Transpose());

	// UV変換行列をマテリアルに設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	cube.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプライン・ルートシグネチャ・プリミティブ設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Standard3D"));
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &cube.mesh.GetVertexBufferView());
	commandManager_->GetCommandList()->IASetIndexBuffer(&cube.mesh.GetIndexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, cube.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, cube.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle)); // テクスチャ
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(envMapSrvHandle)); // 環境マップ
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(cube.mesh.GetIndexCount()), 1, 0, 0, 0);
	indexCube_++; // 使用カウント更新
}

void Renderer::CreateLines()
{
	lines_.resize(kMaxLineCount);

	for (size_t i = 0; i < kMaxLineCount; ++i)
	{
		lines_[i].materialHandle = materialManager_->CreateMaterial(device_->GetDevice());

		lines_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		lines_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&lines_[i].mappedData));
		lines_[i].mesh.SetVertexCount(2);

		// 空の2頂点バッファを1回だけ作る（後でMap更新）
		std::vector<VertexData> dummyVertices = {
			{}, {}
		};
		lines_[i].mesh.InitializeVertexOnly(device_->GetDevice(), dummyVertices);
	}
	indexLine_ = 0;
}

void Renderer::SubmitLine(const Vector3& start, const Vector3& end, Camera& camera, uint32_t color)
{
	assert(indexLine_ < kMaxLineCount);

	RenderData& line = lines_[indexLine_];

	// 頂点バッファを更新
	VertexData* mappedVertices = nullptr;
	line.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));
	mappedVertices[0] = { { start.x, start.y, start.z, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
	mappedVertices[1] = { { end.x,   end.y,   end.z,   1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
	line.mesh.GetVertexResource()->Unmap(0, nullptr);
	line.mesh.SetVertexCount(2);

	// マテリアルと行列を設定
	line.materialHandle.materialData->color = Math::Uint32ToColorVector(color);
	line.worldMatrix = Matrix4x4::MakeIdentity();
	Matrix4x4 wvpMatrix = line.worldMatrix * camera.GetViewProjectionMatrix();
	memcpy(&line.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));

	// 中点のZで深度を計算
	Vector3 midPoint = { (start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, (start.z + end.z) * 0.5f };
	Matrix4x4 worldView = line.worldMatrix * camera.GetViewMatrix();
	float w = midPoint.x * worldView.m[0][3] + midPoint.y * worldView.m[1][3] + midPoint.z * worldView.m[2][3] + worldView.m[3][3];
	float z = (midPoint.x * worldView.m[0][2] + midPoint.y * worldView.m[1][2] + midPoint.z * worldView.m[2][2] + worldView.m[3][2]) / w;

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Line;
	submission.instanceIndex = indexLine_;
	submission.group = RenderGroup::Opaque;
	submission.depth = z;
	submission.materialHandle = line.materialHandle;
	submission.color = color;

	modelSubmissions_.push_back(submission);
	indexLine_++;
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

	frameDataResource_ = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(FrameData));
	frameDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&frameData_));
}

void Renderer::SubmitParticleInstance(WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ,
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

void Renderer::DrawParticles(const Camera& camera)
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
	frameData_->viewProjectionMatrix = camera.GetViewProjectionMatrix();
	Matrix4x4 view = camera.GetViewMatrix();
	frameData_->cameraRight = { view.m[0][0], view.m[1][0], view.m[2][0] };
	frameData_->cameraUp = { view.m[0][1], view.m[1][1], view.m[2][1] };
	cmdList->SetGraphicsRootConstantBufferView(1, frameDataResource_->GetGPUVirtualAddress());

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

	// ShapeGenerator を使ってメッシュデータを生成
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

void Renderer::SubmitSkybox(Camera& camera, WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
	// WVP行列の計算（カメラの位置を除去して回転のみ反映）
	Matrix4x4 viewMatrix = camera.GetViewMatrix();
	Matrix4x4 projectionMatrix = camera.GetProjectionMatrix();
	viewMatrix.m[3][0] = 0.0f;
	viewMatrix.m[3][1] = 0.0f;
	viewMatrix.m[3][2] = 0.0f;

	Matrix4x4 worldMatrix = worldTransform.matWorld_;
	Matrix4x4 wvpMatrix = worldMatrix * viewMatrix * projectionMatrix;
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
	trails_.resize(kMaxTrailCount);

	for (size_t i = 0; i < kMaxTrailCount; ++i)
	{
		// WVP バッファ作成とマッピング
		trails_[i].wvpResource =
			BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		trails_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&trails_[i].mappedWvp));

		// マテリアルバッファ作成とマッピング
		trails_[i].materialResource =
			BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TrailMaterialData));
		trails_[i].materialResource->Map(0, nullptr, reinterpret_cast<void**>(&trails_[i].mappedMaterial));

		// 頂点バッファ作成（最大数確保）
		std::vector<VertexDataTrail> dummyVertices(kMaxTrailVertices);
		trails_[i].mesh.InitializeVertexTrail(device_->GetDevice(), dummyVertices);
	}

	indexTrail_ = 0;
}
void Renderer::SubmitTrail(const std::vector<TrailPoint>& points, const TrailModule& config, Camera& camera)
{
	if (indexTrail_ >= kMaxTrailCount) return;
	if (points.size() < 2) return;

	TrailRenderData& trailData = trails_[indexTrail_];

	// 頂点生成
	std::vector<VertexDataTrail> vertices;
	vertices.reserve(points.size() * 2);

	// Tile用の距離計算
	std::vector<float> distances;
	float totalLength = 0.0f;
	if (config.textureMode == TrailTextureMode::Tile)
	{
		distances.resize(points.size());
		distances[0] = 0.0f;
		for (size_t i = 0; i < points.size() - 1; ++i)
		{
			float d = (points[i + 1].position - points[i].position).Length();
			totalLength += d;
			distances[i + 1] = totalLength;
		}
	}

	Vector3 cameraPos = camera.GetTranslation();

	// 頂点データ作成（左右位置、UV、色）
	for (size_t i = 0; i < points.size(); ++i)
	{
		if (vertices.size() >= kMaxTrailVertices) break;

		Vector3 currentPos = points[i].position;

		// 進行方向
		Vector3 forward = (i < points.size() - 1) ? points[i + 1].position - currentPos : currentPos - points[i - 1].position;
		forward = forward.Normalize();

		// 横方向
		Vector3 right;
		if (config.alignment == TrailAlignment::View)
		{
			Vector3 toCamera = (cameraPos - points[i].position).Normalize();
			right = Math::CrossProduct(toCamera, forward).Normalize();
		}
		else
		{
			Vector3 upVector = points[i].rotationQuaternion.RotateVector({ 0.0f, 1.0f, 0.0f });
			right = Math::CrossProduct(upVector, forward).Normalize();
		}

		if (right.LengthSq() < 0.001f)
		{
			right = Math::CrossProduct({ 0.0f, 1.0f, 0.0f }, forward);

			if (right.LengthSq() < 0.001f)
			{
				right = Math::CrossProduct({ 1.0f, 0.0f, 0.0f }, forward);
			}

			right = right.Normalize();
		}

		// 幅と頂点位置
		float u_norm = static_cast<float>(i) / (points.size() - 1);
		float widthScale = std::lerp(config.tailWidthScale, config.headWidthScale, u_norm);
		float currentWidth = config.width * widthScale;

		Vector3 posLeft = currentPos - right * (currentWidth * 0.5f);
		Vector3 posRight = currentPos + right * (currentWidth * 0.5f);

		// UV計算
		float texU = (config.textureMode == TrailTextureMode::Stretch) ? u_norm * config.tiling.x : distances[i] * config.tiling.x;

		// 色補間
		Vector4 color;
		color.x = std::lerp(config.endColor.x, config.startColor.x, u_norm);
		color.y = std::lerp(config.endColor.y, config.startColor.y, u_norm);
		color.z = std::lerp(config.endColor.z, config.startColor.z, u_norm);
		color.w = std::lerp(config.endColor.w, config.startColor.w, u_norm);

		vertices.push_back({ { posLeft.x, posLeft.y, posLeft.z, 1.0f }, { texU, 0.0f }, color });
		vertices.push_back({ { posRight.x, posRight.y, posRight.z, 1.0f }, { texU, 1.0f }, color });
	}

	// 頂点バッファ更新
	VertexDataTrail* mappedVertices = nullptr;
	trailData.mesh.GetVertexResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices));
	memcpy(mappedVertices, vertices.data(), sizeof(VertexDataTrail) * vertices.size());
	trailData.mesh.GetVertexResource()->Unmap(0, nullptr);
	trailData.mesh.SetVertexCount(static_cast<uint32_t>(vertices.size()));

	// 行列設定
	Matrix4x4 worldMat = Matrix4x4::MakeIdentity();
	Matrix4x4 wvpMat = worldMat * camera.GetViewProjectionMatrix();
	trailData.mappedWvp->WVP = wvpMat;
	trailData.mappedWvp->World = worldMat;

	// マテリアル設定
	trailData.mappedMaterial->scrollSpeed = config.scrollSpeed;
	trailData.mappedMaterial->jitterStrength = config.jitterStrength;
	trailData.mappedMaterial->jitterFrequency = config.jitterFrequency;
	trailData.mappedMaterial->jitterSpeed = config.jitterSpeed;
	trailData.mappedMaterial->jitterMode = static_cast<int>(config.jitterMode);
	trailData.mappedMaterial->jitterPhase = config.jitterPhase;
	trailData.mappedMaterial->isDissolveEnabled = (config.dissolveTextureID >= 0) ? 1 : 0;
	trailData.mappedMaterial->emissiveIntensity = config.emissiveIntensity;

	// 深度計算（半透明ソート用）
	Vector3 midPos = (points.front().position + points.back().position) * 0.5f;
	Matrix4x4 worldView = worldMat * camera.GetViewMatrix();
	float w = midPos.x * worldView.m[0][3] + midPos.y * worldView.m[1][3] + midPos.z * worldView.m[2][3] + worldView.m[3][3];
	float z = (midPos.x * worldView.m[0][2] + midPos.y * worldView.m[1][2] + midPos.z * worldView.m[2][2] + worldView.m[3][2]) / w;

	// ディゾルブテクスチャ
	uint32_t dissolveHandle = 0;
	if (config.dissolveTextureID >= 0)
	{
		dissolveHandle = ParticleTextureHandle::Get(static_cast<ParticleTextureID>(config.dissolveTextureID));
	}

	// 描画キューに登録
	ModelSubmission submission{};
	submission.type = RenderType::Trail;
	submission.instanceIndex = indexTrail_;
	submission.group = RenderGroup::Trail;
	submission.depth = z;
	uint32_t mainTexHandle = ParticleTextureHandle::Get(config.textureID);
	submission.textureHandle = mainTexHandle;
	submission.envMapSrvHandle = dissolveHandle;// Dissolveテクスチャ用

	modelSubmissions_.push_back(submission);
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

	cmdList->DrawIndexedInstanced(UINT(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
}
void Renderer::DrawModel(const ModelSubmission& sub)
{
	Mesh* mesh = GetOrCreateMesh(*sub.modelData);
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
	else // 通常モデル
	{
		if (isWireFrame_)
		{
			psoName = "Wireframe";
		}
		else {
			switch (sub.blendMode)
			{
			case BlendMode::kBlendModeAdd:      psoName = "Object3DAdd";   break;
			case BlendMode::kBlendModeNormal:   psoName = "Object3DTransparent"; break;
			case BlendMode::kBlendModeNone:
			default:                            psoName = "Standard3D";       break; // 通常
			}
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
			cmdList->SetGraphicsRootConstantBufferView(2, buffer.outlineResource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(3, frameDataResource_->GetGPUVirtualAddress());

			D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), sub.skinCluster->influenceBufferView };
			cmdList->IASetVertexBuffers(0, 2, vbvs);
		}
		else
		{
			cmdList->SetPipelineState(psoManager_->GetPSO("Object3DOutline"));
			cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Outline"));

			cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(1, buffer.outlineResource->GetGPUVirtualAddress());
			cmdList->SetGraphicsRootConstantBufferView(2, frameDataResource_->GetGPUVirtualAddress());
		}

		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
		cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	}

	// スキニング描画
	if (isSkinning)
	{
		cmdList->SetPipelineState(psoManager_->GetPSO("Skinning"));
		cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skinning"));

		D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), sub.skinCluster->influenceBufferView };
		cmdList->IASetVertexBuffers(0, 2, vbvs);
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		sub.materialHandle.materialData->color = Math::Uint32ToColorVector(sub.color);

		cmdList->SetGraphicsRootConstantBufferView(0, buffer.wvpResource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(sub.skinCluster->paletteSrvIndex));
		cmdList->SetGraphicsRootConstantBufferView(2, sub.materialHandle.resource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.textureHandle));
		cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(sub.envMapSrvHandle));
		cmdList->SetGraphicsRootConstantBufferView(5, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(6, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(9, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(10, shadowMap_->GetSRVHandle());
		cmdList->SetGraphicsRootDescriptorTable(11, srvManager_->GetSRVHandleGPU(sub.toonRampHandle));
	}

	// 通常モデル描画
	else
	{
		cmdList->SetPipelineState(psoManager_->GetPSO(psoName));
		cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));

		cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		sub.materialHandle.materialData->color = Math::Uint32ToColorVector(sub.color);

		cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));
		cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.envMapSrvHandle));
		cmdList->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(9, shadowMap_->GetSRVHandle());
		cmdList->SetGraphicsRootDescriptorTable(10, srvManager_->GetSRVHandleGPU(sub.toonRampHandle));
	}

	cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
}
void Renderer::DrawGrid(const ModelSubmission& sub)
{
	Mesh* mesh = GetOrCreateMesh(*sub.modelData);
	auto& buffer = perObjectBuffers_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetPipelineState(psoManager_->GetPSO("Grid"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));

	cmdList->IASetIndexBuffer(&mesh->GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());

	cmdList->SetGraphicsRootConstantBufferView(0, sub.materialHandle.resource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, buffer.wvpResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));
	cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.textureHandle));

	cmdList->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootDescriptorTable(9, shadowMap_->GetSRVHandle());
	cmdList->SetGraphicsRootDescriptorTable(10, srvManager_->GetSRVHandleGPU(0));

	cmdList->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
}

void Renderer::DrawLine(const ModelSubmission& sub)
{
	RenderData& line = lines_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();

	// PSOとルートシグネチャを設定
	cmdList->SetPipelineState(psoManager_->GetPSO("Line"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Line"));

	// 線描画用のプリミティブトポロジーを設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// 頂点バッファをセット（インデックスバッファなし）
	cmdList->IASetVertexBuffers(0, 1, &line.mesh.GetVertexBufferView());

	// 定数バッファをセット
	cmdList->SetGraphicsRootConstantBufferView(0, line.materialHandle.resource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, line.wvpResource->GetGPUVirtualAddress());

	// 描画
	cmdList->DrawInstanced(UINT(line.mesh.GetVertexCount()), 1, 0, 0);
}

void Renderer::DrawTrail(const ModelSubmission& sub)
{
	TrailRenderData& trailData = trails_[sub.instanceIndex];
	auto* cmdList = commandManager_->GetCommandList();

	// パイプラインとルートシグネチャ設定
	cmdList->SetPipelineState(psoManager_->GetPSO("Trail"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Trail"));

	// プリミティブトポロジー設定（TRIANGLESTRIP）
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &trailData.mesh.GetVertexBufferView());

	// 定数バッファ設定（WVP、マテリアル、フレームデータ）
	cmdList->SetGraphicsRootConstantBufferView(0, trailData.wvpResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, trailData.materialResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(2, frameDataResource_->GetGPUVirtualAddress());

	// テクスチャ設定（メイン + ディゾルブ）
	cmdList->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(sub.textureHandle));
	uint32_t dissolveHandle = sub.envMapSrvHandle;
	if (trailData.mappedMaterial->isDissolveEnabled)
		cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(dissolveHandle));
	else
		cmdList->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(sub.textureHandle));

	// 描画
	cmdList->DrawInstanced(UINT(trailData.mesh.GetVertexCount()), 1, 0, 0);
}

void Renderer::DrawSkybox(const ModelSubmission& sub)
{
	auto* cmdList = commandManager_->GetCommandList();

	// パイプラインステートとルートシグネチャ設定
	cmdList->SetPipelineState(psoManager_->GetPSO("Skybox"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skybox"));

	// プリミティブトポロジー設定（三角形リスト）
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// メッシュの頂点・インデックスバッファ設定
	cmdList->IASetVertexBuffers(0, 1, &skyboxMesh_.GetVertexBufferView());
	cmdList->IASetIndexBuffer(&skyboxMesh_.GetIndexBufferView());

	// ルートパラメータ設定
	cmdList->SetGraphicsRootConstantBufferView(0, skyboxMaterialHandle_.resource->GetGPUVirtualAddress()); // マテリアルカラー
	cmdList->SetGraphicsRootConstantBufferView(1, skyboxWvpResource_->GetGPUVirtualAddress());              // WVP行列
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(sub.textureHandle));             // キューブテクスチャ

	// 描画
	cmdList->DrawIndexedInstanced(static_cast<UINT>(skyboxMesh_.GetIndexCount()), 1, 0, 0, 0);
}