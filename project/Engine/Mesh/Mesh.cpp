#include "Mesh.h"
#include <cstring>

void Mesh::InitializeVertexOnly(ID3D12Device* device, const std::vector<VertexData>& vertices)
{
	vertexCount_ = vertices.size();
	size_t bufferSize = sizeof(VertexData) * vertexCount_;

	// バッファ作成
	vertexResource_ = BufferManager::CreateBufferResource(device, bufferSize);

	// データ転送
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	std::memcpy(vertexData_, vertices.data(), bufferSize);

	// 頂点バッファビューの作成
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズ
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertices.size());
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

void Mesh::Initialize(ID3D12Device* device, const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices)
{
	// 頂点バッファの作成
	InitializeVertexOnly(device, vertices);

	indexCount_ = indices.size();
	size_t indexBufferSize = sizeof(uint32_t) * indexCount_;

	indexResource_ = BufferManager::CreateBufferResource(device, indexBufferSize);
	
	uint32_t* mappedIndex = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndex));
	std::memcpy(mappedIndex, indices.data(), indexBufferSize);
	indexResource_->Unmap(0, nullptr);

	// インデックスバッファービューの生成
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズ
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indices.size());
	// インデックスはuint32_tとする
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Mesh::InitializeVertexTrail(ID3D12Device* device, const std::vector<VertexDataTrail>& vertices)
{
	// Trailは動的に変わるので頂点数を保存
	vertexCount_ = vertices.size();

	// VertexDataTrail のサイズで計算する
	size_t bufferSize = sizeof(VertexDataTrail) * vertexCount_;

	// バッファ作成
	vertexResource_ = BufferManager::CreateBufferResource(device, bufferSize);

	// データ転送
	// (Trailは毎フレーム更新するので、ここは空のデータでも良いが、初期値を入れておく)
	VertexDataTrail* mappedData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
	std::memcpy(mappedData, vertices.data(), bufferSize);
	vertexResource_->Unmap(0, nullptr); // Mapしっぱなしにする設計ならUnmapしない

	// 頂点バッファビューの作成
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
	// ストライド（1頂点の幅）を VertexDataTrail に合わせる
	vertexBufferView_.StrideInBytes = sizeof(VertexDataTrail);
}