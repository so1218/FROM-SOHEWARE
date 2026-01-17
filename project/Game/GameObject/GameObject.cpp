#include "GameObject.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"

GameObject::GameObject(Engine* engine, Camera* camera)
    : engine_(engine), camera_(camera)
{
}

std::unique_ptr<Model> GameObject::CreateModel(ModelID modelID)
{
	return std::make_unique<Model>(engine_, camera_, ModelHandle::Get(modelID));
}

std::unique_ptr<AnimationModel> GameObject::CreateAnimationModel(ModelID modelID, AnimationID animationID)
{
	return std::make_unique<AnimationModel>(engine_, camera_, ModelHandle::Get(modelID), AnimationHandle::Get(animationID));
}

std::unique_ptr<Sprite> GameObject::CreateSprite(uint32_t textureHandle)
{
    auto sprite = std::make_unique<Sprite>(engine_);

    sprite->SetTexture((TextureID)textureHandle);

    return sprite;
}