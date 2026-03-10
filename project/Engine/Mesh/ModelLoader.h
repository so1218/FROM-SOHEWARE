#pragma once
#include "Structures.h"

class ModelLoader
{
public:
    // ファイルパスを受け取り、ModelDataを返す主要なロード関数
    ModelData LoadModel(const std::string& filePath);

private:
    // Assimpのメッシュを処理し、ModelDataに変換するヘルパー関数
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, MeshData& outMeshData, bool isGLTF);

    // マテリアルを読み込むヘルパー関数
    void LoadMaterialForMesh(const aiScene* scene, aiMesh* mesh, MeshData& outMeshData, const std::string& directoryPath);

    bool IsGLTFFile(const std::string& path);

    Node ReadNode(aiNode* node);

    void CalculateSmoothNormals(std::vector<VertexData>& vertices);
};

