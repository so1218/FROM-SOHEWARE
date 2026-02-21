#include "GameObject.h"
#include "ModelManager.h"
#include "AnimationManager.h"

GameObject::GameObject(Engine* engine, int priority)
    : engine_(engine), priority_(priority)
{
}

std::unique_ptr<Model> GameObject::CreateModel(const std::string& modelName)
{
    // ModelManagerから取得
    const ModelData* modelData = ModelManager::GetInstance().Get(modelName);

    // 見つからなかった場合の安全策
    if (!modelData)
    {
        assert(false && "Model not found");
        return nullptr;
    }

    return std::make_unique<Model>(engine_, modelData);
}

std::unique_ptr<AnimationModel> GameObject::CreateAnimationModel(const std::string& modelName, const std::string& animationName)
{
    // ModelManagerから取得
    const ModelData* modelData = ModelManager::GetInstance().Get(modelName);

    // AnimationManagerから取得
    const Animation* animation = AnimationManager::GetInstance()->Get(animationName);

    // 両方存在するかチェック
    if (!modelData || !animation) 
    {
        assert(false && "Model or Animation not found");
        return nullptr;
    }

    return std::make_unique<AnimationModel>(engine_, modelData, animation);
}

std::unique_ptr<Sprite> GameObject::CreateSprite(const std::string& textureName)
{
    // スプライトインスタンス生成
    auto sprite = std::make_unique<Sprite>(engine_);

    // 文字列でテクスチャを指定
    sprite->SetTexture(textureName);

    return sprite;
}