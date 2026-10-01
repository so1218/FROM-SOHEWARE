#pragma once
#include "Engine.h"
#include "GameObject.h"
#include "AnimationModel.h"
#include "Collider.h"
#include "Terrain.h"
#include "PropertyBinder.h"

class Zombi : public FE::GameObject
{
public:
	Zombi(FE::Engine* engine);

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void DebugDraw() override;

	void OnCollisionEnter(FE::Collider* mine, FE::Collider* other) override;

private:
	FE::Engine* engine_ = nullptr;
	FE::Terrain* terrain_ = nullptr;

	std::unique_ptr<FE::AnimationModel> animationModel_ = nullptr;
	std::unique_ptr<FE::Collider> collider_ = nullptr;
	std::unique_ptr<FE::PropertyBinder> binder_ = nullptr;
};