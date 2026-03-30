#include "pch.h"
#include "Bubble.h"
#include "ImGuiManager.h"

using namespace FE;

Bubble::Bubble(Engine* engine) : GameObject(engine)
{
	SetTag("Bubble");
	model_ = GameObject::CreateModel("sphere");

	binder_ = std::make_unique<PropertyBinder>(engine_, "Bubble");
}

void Bubble::Initialize()
{
	binder_->BindModel("bubbleModel", model_.get());
	auto* bubbleMat = model_->GetMaterialData();
	binder_->Bind("wobbleAmplitude", &bubbleMat->wobbleAmplitude, 1.0f);
	binder_->Bind("wobbleSpeed", &bubbleMat->wobbleSpeed, 5.0f);
	binder_->Bind("fresnelExponent", &bubbleMat->fresnelExponent, 2.0f);
	binder_->Bind("rainbowIntensity", &bubbleMat->rainbowIntensity, 1.0f);
	model_->GetMaterialData()->isBubble = true;
	model_->SetBlendMode(BlendMode::kBlendModeNormal);
};

void Bubble::Update()
{

};

void Bubble::Draw()
{
	model_->Draw();
};

void Bubble::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("バブル");

	binder_->DrawModel("bubbleModel", "インスペクター");

	ImGui::Separator();

	binder_->Draw("wobbleAmplitude", "動く距離");
	binder_->Draw("wobbleSpeed", "波打つ速さ");
	binder_->Draw("fresnelExponent", "グラデーション");
	binder_->Draw("rainbowIntensity", "虹色の輝度");

	ImGui::End();
#endif
}