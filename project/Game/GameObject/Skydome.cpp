#include "pch.h"
#include "Skydome.h"
#include "ImGuiManager.h"

using namespace FE;

Skydome::Skydome(Engine* engine) : GameObject()
{
	engine_ = engine;
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