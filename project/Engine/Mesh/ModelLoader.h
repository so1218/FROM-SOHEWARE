#pragma once

#include "Structures.h"
#include "Engine.h"

#include <fstream>
#include <sstream>
#include <string> 
#include <vector> 
#include <cassert>
#include <cstdint>

#include <assimp/Importer.hpp>   
#include <assimp/scene.h>        
#include <assimp/postprocess.h> 

class ModelLoader
{
public:
    // ファイルパスを受け取り、ModelDataを返す主要なロード関数
    ModelData LoadModel(const std::string& filePath);
    std::vector<ModelData> LoadMultiModel(const std::string& filePath, Engine* engine);

private:
    // Assimpのメッシュを処理し、ModelDataに変換するヘルパー関数
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, ModelData& modelData, bool isGLTF);

    // マテリアルを読み込むヘルパー関数
    void LoadMaterials(const aiScene* scene, ModelData& modelData, const std::string& directoryPath);

    bool IsGLTFFile(const std::string& path);

    Node ReadNode(aiNode* node);
    
    void CalculateSmoothNormals(std::vector<VertexData>& vertices);
};

