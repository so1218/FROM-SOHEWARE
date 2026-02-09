#pragma once
#include <string>
#include <unordered_map>
#include <cassert>
#include "AnimationLoader.h" 

class AnimationManager
{
public:
    static AnimationManager& GetInstance() 
    {
        static AnimationManager instance;
        return instance;
    }

    // コピー禁止
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // ロード関数
    void Load(const std::string& name, const std::string& path);

    // 取得関数
    const Animation* GetAnimation(const std::string& name);

private:
    AnimationManager() = default;
    ~AnimationManager() = default;

    // 文字列でアニメーションデータを管理
    std::unordered_map<std::string, Animation> animations_;
};