#include "pch.h"
#include "AnimationManager.h"
#include "AssetFiles.h"

namespace FS
{

AnimationManager* AnimationManager::GetInstance()
{
    static AnimationManager instance;
    return &instance;
}

void AnimationManager::LoadAllAnimations()
{
    for (const std::filesystem::path& path : CollectAssetFiles("Models", { ".gltf" }))
    {
        // メッシュがある glTF は ModelManager が読む
        if (ContainsMesh(path)) continue;

        Load(GetAssetName(path), path.generic_string());
    }
}

void AnimationManager::Load(const std::string& name, const std::string& path)
{
    if (animations_.contains(name))
    {
        std::cerr << "[AnimationManager] Duplicate animation name, skipped: " << path << std::endl;
        return;
    }

    // 実データのロード
    animations_[name] = LoadAnimationFile(path.c_str());
}

const Animation* AnimationManager::Get(const std::string& name)
{
    auto it = animations_.find(name);
    if (it == animations_.end())
    {
        return nullptr;
    }
    return &it->second;
}

}