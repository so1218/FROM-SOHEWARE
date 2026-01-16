#include "Skydome.h"
#include "ImGuiManager.h"

Skydome::Skydome(Engine* engine, Camera* camera) : GameObject(engine, camera)
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
	ImGui::Begin("天球");

	ImGui::End();
}