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
const int32_t Renderer::kMaxLineCount = 400;// ラインの最大数
const int32_t Renderer::kMaxParticleCount = 8000;// パーティクルの最大数

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
    int clientWidth, int clientHeight)
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

	// DrawCall発行（3頂点の三角形）
	cmdList->DrawInstanced(3, 1, 0, 0);
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

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sphere.mesh.GetIndexCount()), 1, 0, 0, 0);

	indexSphere_++; // 使用カウント更新
}

Mesh* Renderer::GetOrCreateMesh(const ModelData& modelData)
{
	auto it = meshCache.find(&modelData);
	if (it != meshCache.end()) {
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
	models_.resize(kMaxModelCount);

	for (size_t i = 0; i < kMaxModelCount; ++i)
	{
		// WVP行列用のバッファを作成
		models_[i].wvpResource = BufferManager::CreateBufferResource(
			device_->GetDevice(), sizeof(TransformationMatrix));
		models_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&models_[i].mappedData));
	}
	indexModel_ = 0;
}

void Renderer::DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t envMapSrvHandle, uint32_t color, MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount); // 配列範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& model = models_[indexModel_];

	// Meshの取得（キャッシュ）
	Mesh* mesh = GetOrCreateMesh(modelData);

	// マテリアル色を設定
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列を設定
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列（World * ViewProjection）を計算してGPUバッファにコピー
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャ・パイプライン設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->SetPipelineState(isWireFrame_ ? psoManager_->GetPSO("Wireframe") : psoManager_->GetPSO("Standard3D"));

	// プリミティブ・バッファ設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialHandle.resource->GetGPUVirtualAddress()); // Material
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());       // WVP
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle)); // Texture
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(envMapSrvHandle)); // Environment Map
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);

	indexModel_++; // 使用カウント更新
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

			DrawLine(parentPos, childPos, camera, color);
		}
	}
}

void Renderer::DrawAnimationModel(
	WorldTransform& worldTransform,
	Camera& camera,
	const AnimatedModelData& instance,
	const SkinCluster& skinCluster,
	uint32_t textureHandle,
	uint32_t envMapSrvHandle,
	uint32_t color,
	MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount); // 配列範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& model = models_[indexModel_];
	Mesh* mesh = GetOrCreateMesh(instance.modelData);

	// マテリアル色を設定
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列を設定
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列を計算してGPUバッファにコピー
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャ・パイプライン設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skinning"));
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Skinning"));

	// プリミティブ・バッファ設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, model.wvpResource->GetGPUVirtualAddress()); // VS WVP
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(skinCluster.paletteSrvIndex)); // VS MatrixPalette
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(2, materialHandle.resource->GetGPUVirtualAddress()); // PS Material
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(textureHandle)); // PS Texture
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(4, srvManager_->GetSRVHandleGPU(envMapSrvHandle)); // PS Environment
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(9, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// 頂点バッファ設定（通常頂点 + スキンインフルエンス頂点）
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { mesh->GetVertexBufferView(), skinCluster.influenceBufferView };
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);

	indexModel_++; // 使用カウント更新
}

void Renderer::DrawGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color, MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount); // モデル配列の範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& model = models_[indexModel_];
	Mesh* mesh = GetOrCreateMesh(modelData); // メッシュ取得

	materialHandle.materialData->color = Math::Uint32ToColorVector(color); // 色セット
	model.worldMatrix = worldTransform.matWorld_; // ワールド行列
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャとパイプラインステート
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Grid"));

	// 頂点・インデックスバッファ
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());

	// 定数バッファ・SRVバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle));
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(textureHandle)); // ダミー環境マップ
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	indexModel_++; // 使用カウント更新
}

void Renderer::CreateSprites()
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
		sprites_[i].mesh.Initialize(device_->GetDevice(), spriteVertices, spriteIndices);

		// マテリアルを作成・設定
		sprites_[i].materialHandle = materialManager_->CreateMaterial(device_->GetDevice());
		sprites_[i].materialHandle.materialData->uvTransform = Matrix4x4::MakeIdentity();// UV行列は単位行列で初期化
		// WVP行列用のバッファを作成
		sprites_[i].wvpResource = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(TransformationMatrix));
		sprites_[i].wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&sprites_[i].mappedData));  // CPUアクセス用にマッピング

		sprites_[i].mesh.SetIndexCount(spriteIndices.size()); // インデックス数を設定
	}

	// 最初に使用するスフィアのインデックスをリセット
	indexSprite_ = 0;
}

void Renderer::DrawSprite(Vector2 position, Vector2 size, float rotation, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	assert(indexSprite_ < kMaxSpriteCount); // スプライト配列の範囲チェック

	// 描画用SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& sprite = sprites_[indexSprite_];

	// マテリアル設定
	sprite.materialHandle.materialData->color = Math::Uint32ToColorVector(color);
	sprite.materialHandle.materialData->enableLighting = false;

	// ワールド行列を計算
	Matrix4x4 scaleMatrix = Matrix4x4::MakeScale({ size.x, size.y, 1.0f });
	Matrix4x4 rotationMatrix = Matrix4x4::MakeRotateZ(rotation);
	Matrix4x4 translateMatrix = Matrix4x4::MakeTranslate({ position.x, position.y, 0.0f });
	sprite.worldMatrix = (scaleMatrix * rotationMatrix) * translateMatrix;

	// WVP行列を計算してGPUバッファにコピー
	WorldTransform tempTransform = { {size.x, size.y, 1.0f}, {0.0f, 0.0f, rotation}, {position.x, position.y, 0.0f} };
	Matrix4x4 wvpMatrix = Matrix4x4::MakeWVPMatrix2D(tempTransform, float(clientWidth_), float(clientHeight_));
	memcpy(&sprite.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));

	// UV変換行列をマテリアルに設定
	Matrix4x4 uvTransformMatrix = Matrix4x4::MakeScale(uvTransform.scale_);
	uvTransformMatrix = (uvTransformMatrix * Matrix4x4::MakeRotateZ(uvTransform.rotation_.z)) * Matrix4x4::MakeTranslate(uvTransform.translation_);
	sprite.materialHandle.materialData->uvTransform = uvTransformMatrix;

	// パイプライン・ルートシグネチャ・プリミティブ設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Standard3D"));
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("3D"));
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandManager_->GetCommandList()->IASetIndexBuffer(&sprite.mesh.GetIndexBufferView());
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &sprite.mesh.GetVertexBufferView());

	// 定数バッファ・SRVをGPUにバインド
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, sprite.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, sprite.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textureHandle));
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(textureHandle)); // ダミー
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(8, lightManager_->GetAreaLightResource()->GetGPUVirtualAddress());

	// 描画
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
	indexSprite_++; // 使用カウント更新
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

void Renderer::DrawLine(const Vector3& start, const Vector3& end, Camera& camera, uint32_t color)
{
	assert(indexLine_ < kMaxLineCount);

	// 描画に必要なSRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
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
	line.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// WVP行列更新
	line.worldMatrix = Matrix4x4::MakeIdentity();
	Matrix4x4 wvpMatrix = line.worldMatrix * camera.GetViewProjectionMatrix();
	memcpy(&line.mappedData->WVP, &wvpMatrix, sizeof(TransformationMatrix));

	// パイプライン設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->GetPSO("Line"));
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Line"));

	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 1, &line.mesh.GetVertexBufferView());

	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, line.materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, line.wvpResource->GetGPUVirtualAddress());

	commandManager_->GetCommandList()->DrawInstanced(UINT(line.mesh.GetVertexCount()), 1, 0, 0);

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

	cameraBuffer_ = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(FrameData));
	cameraBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&frameData_));
}

void Renderer::SubmitParticleInstance(WorldTransform& worldTransform, uint32_t color, uint32_t textureIndex, float rotationZ, bool isBillboard)
{
	if (indexInstance_ >= kMaxParticleCount) return;

	ParticleInstanceData& data = mappedInstanceData_[currentFrameIndex_][indexInstance_++];
	data.worldMatrix = worldTransform.matWorld_;

	data.color = Math::Uint32ToColorVector(color);
	data.textureIndex = textureIndex;
	data.rotationZ = rotationZ;
	data.isBillboard = isBillboard ? 1 : 0;
	indexParticle_++;

	particlesByTexture_[textureIndex].push_back(data);
}

void Renderer::DrawParticles(const Camera& camera)
{
	if (indexInstance_ == 0) return;

	auto* cmdList = commandManager_->GetCommandList();

	std::string psoName = GetParticlePSOName(currentBlendMode_);

	ID3D12PipelineState* pso = psoManager_->GetPSO(psoName);
	if (pso == nullptr) 
	{
		// JSONファイル名が間違っているか、JSON定義が不正
		assert(false && "Particle PSO not found. Check JSON file name or definition.");
		return;
	}

	cmdList->SetPipelineState(pso);
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Particle"));
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetIndexBuffer(&particleMesh_.GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &particleMesh_.GetVertexBufferView());

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// カメラ定数バッファ更新
	frameData_->viewProjectionMatrix = camera.GetViewProjectionMatrix();
	Matrix4x4 view = camera.GetViewMatrix();
	frameData_->cameraRight = { view.m[0][0], view.m[1][0], view.m[2][0] };
	frameData_->cameraUp = { view.m[0][1], view.m[1][1], view.m[2][1] };
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
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvManager_->GetSRVHandleGPU(textureIndex);
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

void Renderer::DrawSkybox(Camera& camera, WorldTransform& worldTransform, uint32_t color, uint32_t cubeTextureSrvIndex)
{
	auto* cmdList = commandManager_->GetCommandList();

	// PSO と RootSignature をセット
	cmdList->SetPipelineState(psoManager_->GetPSO("Skybox"));
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->GetRootSignature("Skybox"));

	// SRVヒープをセット
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	// WVP行列の計算
	Matrix4x4 viewMatrix = camera.GetViewMatrix();
	Matrix4x4 projectionMatrix = camera.GetProjectionMatrix();

	// ビュー行列から移動成分を削除
	viewMatrix.m[3][0] = 0.0f;
	viewMatrix.m[3][1] = 0.0f;
	viewMatrix.m[3][2] = 0.0f;

	// 引数の worldTransform をワールド行列として使用 (回転を反映)
	Matrix4x4 worldMatrix = worldTransform.matWorld_;
	Matrix4x4 wvpMatrix = worldMatrix * viewMatrix * projectionMatrix;

	memcpy(mappedSkyboxWvp_, &wvpMatrix, sizeof(TransformationMatrix));

	// マテリアルカラーの設定
	// 引数の color をマテリアルバッファに設定
	skyboxMaterialHandle_.materialData->color = Math::Uint32ToColorVector(color);

	// メッシュ情報をセット
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &skyboxMesh_.GetVertexBufferView());
	cmdList->IASetIndexBuffer(&skyboxMesh_.GetIndexBufferView());

	// ルートパラメータを設定
	// MaterialColor
	cmdList->SetGraphicsRootConstantBufferView(0, skyboxMaterialHandle_.resource->GetGPUVirtualAddress());
	// WVP
	cmdList->SetGraphicsRootConstantBufferView(1, skyboxWvpResource_->GetGPUVirtualAddress());
	// Cube Texture SRV
	cmdList->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(cubeTextureSrvIndex));

	// 描画コマンド
	cmdList->DrawIndexedInstanced(static_cast<UINT>(skyboxMesh_.GetIndexCount()), 1, 0, 0, 0);
}