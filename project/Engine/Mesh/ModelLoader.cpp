#include "pch.h"
#include "ModelLoader.h"
#include "Logger.h"
#include "Engine.h"

namespace FE
{

ModelData ModelLoader::LoadModel(const std::string& filePath)
{
    LOG_INFO("\n-------------------- ModelLoader::LoadModel Start --------------------");
    LOG_INFO("Loading model from: {}", filePath);

    ModelData modelData;
    Assimp::Importer importer;

    std::filesystem::path path(filePath);
    std::string directoryPath = path.parent_path().string();

    bool isGLTF = IsGLTFFile(filePath);

    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("Failed to load model file or scene is incomplete: {}", filePath);
        LOG_ERROR("Assimp error: {}", importer.GetErrorString());
        LOG_ERROR("-------------------- ModelLoader::LoadModel Failed ---------------------\n");

        return modelData;
    }

    if (!scene->HasMeshes())
    {
        LOG_ERROR("Model scene has no meshes: {}", filePath);
    }

    modelData.rootNode = ReadNode(scene->mRootNode);

    // 全メッシュをループ処理
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        MeshData meshPart; // 新しいパーツを作成

        if (!mesh->HasNormals())
        {
            LOG_WARN("Mesh '{}' (Index {}) has no normals. Skipping or using default.", mesh->mName.C_Str(), meshIndex);
        }
        if (!mesh->HasTextureCoords(0))
        {
            LOG_WARN("Mesh '{}' (Index {}) has no texture coordinates (UVs). Skipping or using default.", mesh->mName.C_Str(), meshIndex);
        }

        assert(mesh->HasNormals()); // 法線が無いMeshは非対応
        assert(mesh->HasTextureCoords(0)); // Texcoordが無いMeshは非対応

        // メッシュデータの構築
        ProcessMesh(mesh, scene, meshPart, isGLTF);

        // マテリアルの読み込み
        LoadMaterialForMesh(scene, mesh, meshPart, directoryPath);

        // 構築が完了したmeshPartをmodelDataに保存
        modelData.meshes.push_back(std::move(meshPart));
    }
    // 全てのMeshの処理が終わった後、各メッシュに対して事後計算を行う
    for (auto& meshPart : modelData.meshes)
    {
        CalculateSmoothNormals(meshPart.vertices);
    }

    LOG_INFO("Model loaded successfully: {}", filePath);
    LOG_INFO("-------------------- ModelLoader::LoadModel End ----------------------\n");

    return modelData;
}

void ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, MeshData& outMeshData, bool isGLTF)
{
    // AABB計算用の初期値（floatの最大・最小値）
    Vector3 minPos = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vector3 maxPos = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    // 頂点を追加
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VertexData vertex;

        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        vertex.position.w = 1.0f;

        vertex.position.x *= -1;

        // AABBの更新（反転後の座標）
        minPos.x = (std::min)(minPos.x, vertex.position.x);
        minPos.y = (std::min)(minPos.y, vertex.position.y);
        minPos.z = (std::min)(minPos.z, vertex.position.z);
        maxPos.x = (std::max)(maxPos.x, vertex.position.x);
        maxPos.y = (std::max)(maxPos.y, vertex.position.y);
        maxPos.z = (std::max)(maxPos.z, vertex.position.z);

        if (mesh->HasNormals())
        {
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            vertex.normal.x *= -1.0f;
        }
        else
        {
            vertex.normal = { 0.0f, 1.0f, 0.0f };
        }

        if (mesh->HasTextureCoords(0))
        {
            vertex.texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else
        {
            vertex.texcoord = { 0.0f, 0.0f };
        }

        if (mesh->HasTangentsAndBitangents())
        {
            // TangentもX軸を反転
            vertex.tangent.x = mesh->mTangents[i].x * -1.0f;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;
        }
        else
        {
            // タンジェントがない
            vertex.tangent = { 0.0f, 0.0f, 0.0f };
        }

        if (mesh->HasVertexColors(0))
        {
            vertex.color.x = mesh->mColors[0][i].r;
            vertex.color.y = mesh->mColors[0][i].g;
            vertex.color.z = mesh->mColors[0][i].b;
            vertex.color.w = mesh->mColors[0][i].a;
        }
        else
        {
            vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        }

        outMeshData.vertices.push_back(vertex);
    }

    // 計算したAABBをメッシュデータに保存
    outMeshData.localAABB.min = minPos;
    outMeshData.localAABB.max = maxPos;

    // インデックスを追加（vertexOffsetを足す）
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices == 3)
        {
            outMeshData.indices.push_back(face.mIndices[0]);
            outMeshData.indices.push_back(face.mIndices[2]);
            outMeshData.indices.push_back(face.mIndices[1]);
        }
    }

    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        aiBone* bone = mesh->mBones[boneIndex];
        std::string jointName = bone->mName.C_Str();
        JointWeightData& jointWeightData = outMeshData.skinClusterData[jointName];

        aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
        aiVector3D scale, translate;
        aiQuaternion rotate;
        bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
        Matrix4x4 bindPoseMatrix = Matrix4x4::MakeAffine(
            { scale.x,scale.y,scale.z }, { rotate.x,-rotate.y,-rotate.z,rotate.w }, { -translate.x,translate.y,translate.z }
        );
        jointWeightData.inverseBindPoseMatrix = Matrix4x4::Inverse(bindPoseMatrix);

        // vertexIdもオフセットなしでそのまま使う
        for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
        {
            outMeshData.skinClusterData[jointName].vertexWeights.push_back(
                { bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId }
            );
        }
    }
}

void ModelLoader::LoadMaterialForMesh(const aiScene* scene, aiMesh* mesh, MeshData& outMeshData, const std::string& directoryPath)
{
    // メッシュが参照しているマテリアルのインデックスを確認
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString texturePath;

        // ベースカラーまたはディフューズテクスチャを探す
        if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS ||
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
        {
            // Assimpの文字列をstd::stringに変換
            std::string pathStr = texturePath.C_Str();

            // 埋め込みテクスチャのチェック
            if (pathStr.size() > 0 && pathStr[0] == '*')
            {
                outMeshData.textureData.textureFilePath = "";
                printf("[ModelLoader] Embedded texture found (not supported yet): %s\n", pathStr.c_str());
                return;
            }

            try
            {
                // パスの結合
                std::filesystem::path dir(directoryPath);
                std::filesystem::path file(pathStr);

                std::filesystem::path fullTexturePath = dir / file.filename();

                outMeshData.textureData.textureFilePath = fullTexturePath.string();
            }
            catch (const std::system_error& e)
            {
                // エラー内容を表示してクラッシュを防ぐ
                printf("[Error] Filesystem Error: %s\nPath: %s\n", e.what(), pathStr.c_str());
                outMeshData.textureData.textureFilePath = "";
            }
        }
        else
        {
            outMeshData.textureData.textureFilePath = ""; // テクスチャなし
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

    // Assimpの変換行列からスケール、回転(クォータニオン)、平行移動を抽出
    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);

    // transformにセット
    result.transform.scale_ = { scale.x, scale.y, scale.z };

    // 回転の軸反転と回転方向の補正
    result.transform.rotationQuaternion_ = { rotate.x, -rotate.y, -rotate.z, rotate.w };

    // 平行移動のx軸反転
    result.transform.translation_ = { -translate.x, translate.y, translate.z };

    // ローカル行列を作成
    result.localMatrix = Matrix4x4::MakeAffine(
        result.transform.scale_,
        result.transform.rotationQuaternion_,
        result.transform.translation_
    );

    result.name = node->mName.C_Str();

    result.meshIndices.resize(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        result.meshIndices[i] = node->mMeshes[i];
    }

    // 子ノードも再帰的に読み込み
    result.children.resize(node->mNumChildren);

    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
    {
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }

    return result;
}

void ModelLoader::CalculateSmoothNormals(std::vector<VertexData>& vertices)
{
    // 座標をキーにして法線を蓄積するマップ
    std::map<std::tuple<float, float, float>, Vector3> normalMap;

    // 同じ座標にある頂点の法線を加算
    for (const auto& v : vertices)
    {
        auto key = std::make_tuple(v.position.x, v.position.y, v.position.z);

        // ベクトルの加算
        normalMap[key] += v.normal;
    }

    // 加算された法線を正規化
    for (auto& pair : normalMap)
    {
        pair.second = pair.second.Normalize();
    }

    // 計算結果を各頂点のsmoothNormalに格納
    for (auto& v : vertices)
    {
        auto key = std::make_tuple(v.position.x, v.position.y, v.position.z);
        v.smoothNormal = normalMap[key];
    }
}

}