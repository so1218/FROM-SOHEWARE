#include "GameObject.h"
#include "Model.h"
#include "AnimationModel.h"
#include "Sprite.h"

GameObject::GameObject(Engine* engine)
    : engine_(engine)
{
}

std::unique_ptr<Model> GameObject::CreateModel(ModelID modelID)
{
	return std::make_unique<Model>(engine_, ModelHandle::Get(modelID));
}

std::unique_ptr<AnimationModel> GameObject::CreateAnimationModel(ModelID modelID, AnimationID animationID)
{
	return std::make_unique<AnimationModel>(engine_, ModelHandle::Get(modelID), AnimationHandle::Get(animationID));
}

std::unique_ptr<Sprite> GameObject::CreateSprite(TextureID textureID)
{
    auto sprite = std::make_unique<Sprite>(engine_);

    sprite->SetTexture(textureID);

    return sprite;
}