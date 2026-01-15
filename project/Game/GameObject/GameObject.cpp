#include "GameObject.h"

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