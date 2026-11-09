#pragma once
#include "AnimationLoader.h" 

namespace FS
{

class AnimationManager
{
public:
    // シングルトン取得
    static AnimationManager* GetInstance();

    // コピー禁止
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // フォルダ内の、メッシュを持たない glTF をすべて読み込む。名前は拡張子を除いたファイル名
    // Engine/Assets/Models と Assets/Models のアニメーションをすべて読む
    void LoadAllAnimations();

    // 単体ロード
    void Load(const std::string& name, const std::string& path);

    // 取得関数
    const Animation* Get(const std::string& name);

private:
    AnimationManager() = default;
    ~AnimationManager() = default;

    std::unordered_map<std::string, Animation> animations_;
};

}