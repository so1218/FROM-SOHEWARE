#include "AnimationManager.h"
#include <iostream>

void AnimationManager::Load(const std::string& name, const std::string& path)
{
    // 重複チェック
    if (animations_.find(name) != animations_.end())
    {
        return;
    }

    // LoadAnimationFile関数を使ってロード
    animations_[name] = LoadAnimationFile(path.c_str());
}

const Animation* AnimationManager::GetAnimation(const std::string& name)
{
    auto it = animations_.find(name);
    if (it == animations_.end())
    {
        // エラーログ
        std::cout << "Error: Animation [" << name << "] not found" << std::endl;

        // 安全のためにnullptrを返す
        return nullptr;
    }

    // アドレスを返す
    return &it->second;
}