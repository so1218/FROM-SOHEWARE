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
	TextureManager::TextureResources texResources = textureManager_->UploadTexture(mipImages, textures_);

	// 保存したテクスチャのインデックスを返す
	return static_cast<int>(textures_.size()) - 1;
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
	cmdList->SetPipelineState(psoManager_->psoFullscreen_.Get());

	// ルートシグネチャをセット（フルスクリーン用のルートシグネチャ）
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureFullScreen_.Get());

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
void Renderer::DrawTriangle(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	// indexTriangle_が範囲内であることを確認
	assert(indexTriangle_ < kMaxTriangleCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する三角形を取得
	RenderData& triangle = triangles_[indexTriangle_];

	// マテリアルに色情報を設定
	triangle.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列（スケール・回転・移動）を計算
	Vector3 pivot = { 320.0f, 180.0f, 0.0f }; // 三角形の中心
	triangle.worldMatrix = MakeCenteredAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_, pivot);

	// WVP行列（World * ViewProjection）を計算
	Matrix4x4 wvpMatrix = triangle.worldMatrix * Matrix4x4::MakeOrthographic(0, 0, float(clientWidth_), float(clientHeight_), 0, 100);
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
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawInstanced(UINT(triangle.mesh.GetVertexCount()), 1, 0, 0);
	// 使用カウント上昇
	indexTriangle_++;
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
void Renderer::DrawSphere(WorldTransform& worldTransform, Camera& camera, WorldTransform& uvTransform, uint32_t textureHandle, uint32_t color)
{
	// indexSphere_が範囲内であることを確認
	assert(indexSphere_ < kMaxSphereCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する球体を取得
	RenderData& sphere = spheres_[indexSphere_];

	// マテリアルに色情報を設定
	sphere.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

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
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sphere.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexSphere_++;
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

void Renderer::DrawModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color, MaterialHandle& materialHandle)
{
	// indexModel_が範囲内であることを確認
	assert(indexModel_ < kMaxModelCount);

	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するモデルを取得
	RenderData& model = models_[indexModel_];

	// 一度だけ作られたMeshを使う
	Mesh* mesh = GetOrCreateMesh(modelData);

	// 色変換
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

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
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialHandle.resource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, model.wvpResource->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
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

void Renderer::DrawAnimationModel(WorldTransform& worldTransform, Camera& camera, const AnimatedModelData& instance, const SkinCluster& skinCluster, uint32_t textureHandle, uint32_t color, MaterialHandle& materialHandle)
{
	assert(indexModel_ < kMaxModelCount);

	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	RenderData& model = models_[indexModel_];
	Mesh* mesh = GetOrCreateMesh(instance.modelData);

	// 色変換
	materialHandle.materialData->color = Math::Uint32ToColorVector(color);

	// ワールド行列 (スケール・回転・位置) を計算
	model.worldMatrix = worldTransform.matWorld_;

	// WVP行列 (World * ViewProjection) を計算
	Matrix4x4 wvpMatrix = model.worldMatrix * camera.GetViewProjectionMatrix();
	model.mappedData->WVP = wvpMatrix;
	model.mappedData->World = model.worldMatrix;
	model.mappedData->WorldInverseTranspose = Matrix4x4::Inverse(model.worldMatrix.Transpose());

	// ルートシグネチャの設定
	commandManager_->GetCommandList()->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureSkinning_.Get());
	// パイプラインステートの設定
	commandManager_->GetCommandList()->SetPipelineState(psoManager_->psoSkinning_.Get());
	// プリミティブ形状の設定
	commandManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// インデックスバッファの設定
	commandManager_->GetCommandList()->IASetIndexBuffer(&mesh->GetIndexBufferView());
	// 定数バッファをGPUにバインド
	  // [Index 0] : VS CBV (b0) -> TransformationMatrix (WVP, World, etc.)
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, model.wvpResource->GetGPUVirtualAddress());

	// [Index 1] : VS SRV Table (t0) -> gMatrixPalette
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHandleGPU(skinCluster.paletteSrvIndex));

	// [Index 2] : PS CBV (b0) -> Material
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(2, materialHandle.resource->GetGPUVirtualAddress());

	// [Index 3] : PS SRV Table (t0) -> gTexture
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(3, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));

	// [Index 4] : PS CBV (b1) -> DirectionalLights
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());

	// [Index 5] : PS CBV (b2) -> Camera
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());

	// [Index 6] : PS CBV (b3) -> PointLights
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());

	// [Index 7] : PS CBV (b4) -> SpotLights
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(7, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());

	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
		 mesh->GetVertexBufferView(),
		 skinCluster.influenceBufferView
	};
	// 頂点インフルエンス頂点バッファをセット
	commandManager_->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);

	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexModel_++;
}

void Renderer::DrawGrid(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, uint32_t textureHandle, uint32_t color)
{
	// indexModel_が範囲内であることを確認
	assert(indexModel_ < kMaxModelCount);


	// 描画に必要なSRVヒープをセット（モデル描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するモデルを取得
	RenderData& model = models_[indexModel_];

	// 一度だけ作られたMeshを使う
	Mesh* mesh = GetOrCreateMesh(modelData);

	// 色変換
	modelData.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

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
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
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
	// indexSprite_が範囲外でないことを確認
	assert(indexSprite_ < kMaxSpriteCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画するスプライトを取得
	RenderData& sprite = sprites_[indexSprite_];

	// マテリアルに色情報を設定
	sprite.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

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
	Matrix4x4 wvpMatrix = Matrix4x4::MakeWVPMatrix2D(tempTransform, float(clientWidth_), float(clientHeight_));

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
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(sprite.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
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
void Renderer::DrawCube(WorldTransform& worldTransform, uint32_t color, WorldTransform& uvTransform, uint32_t textureHandle)
{
	// indexCube_が範囲内であることを確認
	assert(indexCube_ < kMaxCubeCount);

	// 描画に必要なSRVヒープをセット（描画に必要なヒープに切り替え）
	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
	commandManager_->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// 描画する立方体を取得
	RenderData& cube = cubes_[indexCube_];

	// 色変換
	cube.materialHandle.materialData->color = Math::Uint32ToColorVector(color);

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
	commandManager_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHandleGPU(textures_[textureHandle].srvIndex));
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, lightManager_->GetDirectionalLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(4, cameraManager_->GetCameraResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(5, lightManager_->GetPointLightResource()->GetGPUVirtualAddress());
	commandManager_->GetCommandList()->SetGraphicsRootConstantBufferView(6, lightManager_->GetSpotLightResource()->GetGPUVirtualAddress());
	// 描画コマンド
	commandManager_->GetCommandList()->DrawIndexedInstanced(UINT(cube.mesh.GetIndexCount()), 1, 0, 0, 0);
	// 使用カウント上昇
	indexCube_++;
}

void Renderer::CreateLines()
{
	lines_.resize(kMaxLineCount);

	for (size_t i = 0; i < kMaxLineCount; ++i)
	{
		lines_[i].materialHandle = materialManager_->CreateLineMaterial(device_->GetDevice());

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
	line.materialHandle.lineMaterialData->color = Math::Uint32ToColorVector(color);

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

	cameraBuffer_ = BufferManager::CreateBufferResource(device_->GetDevice(), sizeof(CameraBuffer));
	cameraBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedCamera_));

	psoManager_->CreateAllParticlePipelines();
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

	// 共通設定（パイプラインやルートシグネチャ、頂点/indexバッファ）
	auto& psoMap = psoManager_->psoParticles_;
	auto it = psoMap.find(currentBlendMode_);
	if (it == psoMap.end()) return;

	cmdList->SetPipelineState(it->second.Get());
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureParticles_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetIndexBuffer(&particleMesh_.GetIndexBufferView());
	cmdList->IASetVertexBuffers(0, 1, &particleMesh_.GetVertexBufferView());

	ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSRVHeap() };
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
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvManager_->GetSRVHandleGPU(textures_[textureIndex].srvIndex);
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
	cmdList->SetPipelineState(psoManager_->psoSkybox_.Get());
	cmdList->SetGraphicsRootSignature(rootSignatureManager_->rootSignatureSkybox_.Get());

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