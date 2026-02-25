#include "Ground.h"
#include "ImGuiManager.h"

Ground::Ground(Engine* engine) : GameObject(engine)
{
	SetTag("Ground");

	model_ = GameObject::CreateModel("field");

	binder_ = std::make_unique<PropertyBinder>(engine_, "Ground");
}

void Ground::Initialize()
{
	binder_->BindModel("Model", model_.get());
	
};

void Ground::Update()
{

};

void Ground::Draw()
{
	model_->Draw();
};

void Ground::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("地面");
	binder_->DrawModel("Model", "インスペクター");
	ImGui::End();
#endif
}