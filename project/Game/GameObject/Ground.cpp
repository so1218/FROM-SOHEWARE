#include "Ground.h"
#include "ImGuiManager.h"

Ground::Ground(Engine* engine, Camera* camera) : GameObject(engine, camera)
{
}

void Ground::Initialize()
{

};

void Ground::Update()
{

};

void Ground::Draw()
{

};

void Ground::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("地面");

	ImGui::End();
#endif
}