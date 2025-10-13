#include "ModelLoader.h"
#include <filesystem> 

ModelData ModelLoader::LoadModel(const std::string& filePath)
{
    ModelData modelData;
    Assimp::Importer importer;

    std::filesystem::path path(filePath);
    std::string directoryPath = path.parent_path().string();

    bool isGLTF = IsGLTFFile(filePath);

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        return modelData;
    }

    assert(scene->HasMeshes()); // メッシュが無いのは対応しない

    LoadMaterials(scene, modelData, directoryPath);
	modelData.rootNode = ReadNode(scene->mRootNode);

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        assert(mesh->HasNormals()); // 法線が無いMeshは非対応
        assert(mesh->HasTextureCoords(0)); // Texcoordが無いMeshは非対応
        ProcessMesh(mesh, scene, modelData, isGLTF);
    }

    return modelData;
}

std::vector<ModelData> ModelLoader::LoadMultiModel(const std::string& filePath, Engine* engine)
{
    std::vector<ModelData> modelParts;
    Assimp::Importer importer;

    std::filesystem::path path(filePath);
    std::string directoryPath = path.parent_path().string();

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        return modelParts;
    }

    bool isGLTF = IsGLTFFile(filePath);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        ModelData modelData;

        ProcessMesh(mesh, scene, modelData, isGLTF);

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texturePath;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            {
                std::filesystem::path fullTexturePath = path.parent_path() / texturePath.C_Str();
                modelData.material.textureFilePath = fullTexturePath.string();

                // ここでテクスチャをロードし、ハンドルを取得する例（仮関数）
                modelData.material.textureHandle = engine->LoadTexture(fullTexturePath.string());
            }
        }

        modelData.materialHandle = engine->materialManager_->CreateMaterial(engine->graphicDevice_->GetDevice());

        modelParts.push_back(std::move(modelData));
    }

    return modelParts;
}

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, ModelData& modelData, bool isGLTF)
{
    // 現在の頂点数をオフセットとして記録
    unsigned int vertexOffset = static_cast<unsigned int>(modelData.vertices.size());

    // 頂点を追加
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VertexData vertex;

        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        vertex.position.w = 1.0f;

        if (mesh->HasNormals())
        {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
            
        }
        else
        {
            vertex.normal = { 0.0f, 0.0f, 0.0f };
        }

        if (mesh->HasTextureCoords(0))
        {
            vertex.texcoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texcoord.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.texcoord = { 0.0f, 0.0f };
        }

        modelData.vertices.push_back(vertex);
    }

    // インデックスを追加（vertexOffsetを足す）
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices == 3)
        {
            modelData.indices.push_back(face.mIndices[0] + vertexOffset);
            modelData.indices.push_back(face.mIndices[2] + vertexOffset);
            modelData.indices.push_back(face.mIndices[1] + vertexOffset);
        }
        else
        {
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                modelData.indices.push_back(face.mIndices[j] + vertexOffset);
            }
        }
    }
}

void ModelLoader::LoadMaterials(const aiScene* scene, ModelData& modelData, const std::string& directoryPath)
{
    if (scene->mNumMaterials > 0)
    {
        aiMaterial* material = scene->mMaterials[0]; // 最初のマテリアルを取得

        aiString texturePath;
        // glTF用: ベースカラー取得
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS ||
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            std::filesystem::path fullTexturePath = std::filesystem::path(directoryPath) / texturePath.C_Str();
            modelData.material.textureFilePath = fullTexturePath.string();
        }
        else
        {
            modelData.material.textureFilePath = "";
        }
    }
}

bool ModelLoader::IsGLTFFile(const std::string& path)
{
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".gltf" || ext == ".glb";
}

Node ModelLoader::ReadNode(aiNode* node)
{
    Node result;
	aiMatrix4x4 aiLocalMatrix = node->mTransformation; // nodeのlocalMatrixを取得
    aiLocalMatrix.Transpose(); // 列ベクトル形式を行ベクトル形式に転置

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            result.localMatrix.m[row][col] = aiLocalMatrix[row][col];
        }
    }

	result.name = node->mName.C_Str(); // Node名を格納
    result.children.resize(node->mNumChildren); // 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
	{
        // 再帰的に読んで階層構造を作っていく
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]); 
	}

    return result;
}