#include "Skydome.h"
#include "ImGuiManager.h"

Skydome::Skydome(Engine* engine) : GameObject(engine)
{
}

void Skydome::Initialize()
{

};

void Skydome::Update()
{

};

void Skydome::Draw()
{
	
};

void Skydome::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("天球");

	ImGui::End();
#endif
}