#include "pch.h"
#include "ModelManager.h"
#include "DevError.h"
#include "ModelLoader.h"
#include "AssetFiles.h"
#include "TextureManager.h" 
#include "Engine.h" 
#include "GraphicsDevice.h"
#include "MaterialManager.h"

namespace FS
{

void ModelManager::LoadAllModels(Engine* engine)
{
    engine_ = engine;

    for (const std::filesystem::path& path : CollectAssetFiles("Models", { ".gltf", ".obj" }))
    {
        // メッシュがない glTF は AnimationManager が読む
        if (!ContainsMesh(path)) continue;

        Load(GetAssetName(path), path.generic_string());
    }
}

void ModelManager::Load(const std::string& name, const std::string& path)
{
    if (models_.contains(name))
    {
        std::cerr << "[ModelManager] Duplicate model name, skipped: " << path << std::endl;
        return;
    }

    ModelLoader loader;

    // ModelLoaderの実装に合わせて呼び出し
    ModelData data = loader.LoadModel(path.c_str());

    // ロードした直後に、1回だけマテリアルを作ってキャッシュしておく
    auto& texManager = TextureManager::GetInstance();
    for (size_t i = 0; i < data.meshes.size(); ++i)
    {
        // 1回だけ作成
        MaterialHandle defMat = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

        defMat.textureHandle = texManager.Get("white1x1");
        defMat.envMapHandle = texManager.Get("pureSky");
        defMat.toonRampHandle = texManager.Get("toonRamp_01");
        defMat.dissolveMapHandle = texManager.Get("white1x1");
        defMat.normalMapHandle = texManager.Get("white1x1");

        if (defMat.materialData)
        {
            defMat.materialData->uvTransform = defMat.uvTransformData.MakeMatrix();
        }

        data.defaultMaterials.push_back(defMat);
    }

    // 所有権をunique_ptrに移譲して登録
    models_[name] = std::make_unique<ModelData>(std::move(data));
}

const ModelData* ModelManager::Get(const std::string& name, std::source_location location) const
{
    auto it = models_.find(name);
    if (it == models_.end())
    {
        DevError::Report(std::format("モデル \"{}\" が見つからない\nAssets/Models にその名前のモデルがあるか確認する", name), location);
        return nullptr;
    }
    return it->second.get();
}

std::vector<std::string> ModelManager::GetLoadedModelNames() const
{
    std::vector<std::string> names;
    // unordered_mapからキー（名前）だけを抽出
    for (const auto& pair : models_)
    {
        names.push_back(pair.first);
    }
    return names;
}

void ModelManager::Clear()
{
    models_.clear();
}

}