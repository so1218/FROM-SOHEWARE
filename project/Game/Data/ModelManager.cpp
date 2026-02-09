#include "ModelManager.h"
#include "ModelLoader.h" 
#include <iostream>

void ModelManager::Initialize()
{
}

void ModelManager::Finalize()
{
    models_.clear();
}

void ModelManager::Load(const std::string& name, const std::string& path)
{
    // 重複チェック
    if (models_.find(name) != models_.end())
    {
        return;
    }

    ModelLoader loader;

    // LoadModelが値を返す場合
    ModelData data = loader.LoadModel(path.c_str());

    // unique_ptrに包んで登録
    models_[name] = std::make_unique<ModelData>(std::move(data));
}

const ModelData* ModelManager::GetModel(const std::string& name) const
{
    auto it = models_.find(name);
    if (it == models_.end())
    {
        // 見つからない場合
        std::cerr << "Model not found: " << name << std::endl;
        return nullptr;
    }
    return it->second.get();
}