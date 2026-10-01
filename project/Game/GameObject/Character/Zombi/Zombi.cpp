#include "pch.h"
#include "Zombi.h"
#include "GameDefine.h"

using namespace FE;

Zombi::Zombi(Engine* engine) : engine_(engine)
{
	SetTag(ObjectTag::Enemy);

	animationModel_=std::make_unique<AnimationModel>(engine_, "humanMesh", "humanRun"); 
	binder_ = std::make_unique<PropertyBinder>(engine_, "Zombi");
	collider_ = std::make_unique<Collider>(this);
}

void Zombi::Initialize()
{
	collider_->SetType(CollisionShapeType::AABB);

	animationModel_->Play("humanRun");
}

void Zombi::Update()
{

}

void Zombi::Draw()
{

}

void Zombi::DebugDraw()
{

}

void Zombi::OnCollisionEnter(Collider* mine, Collider* other)
{

}