#pragma once
#include <string>
#include <unordered_map>
#include <cassert>
#include "AnimationLoader.h" 

class AnimationManager
{
public:
    // シングルトン取得
    static AnimationManager* GetInstance();

    // コピー禁止
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // CSVから一括ロード（初期化）
    void LoadFromCSV(const std::string& csvPath = "Assets/Data/AnimationList.csv");

    // 単体ロード（CSVを使わず直接ロードしたい場合用）
    void Load(const std::string& name, const std::string& path);

    // 取得関数
    const Animation* Get(const std::string& name);

private:
    AnimationManager() = default;
    ~AnimationManager() = default;

    std::unordered_map<std::string, Animation> animations_;
};