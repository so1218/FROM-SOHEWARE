#include "Fade.h"
#include "TextureHandle.h"
#include "GlobalVariables.h"
#include "externals/imgui/imgui.h"

#include <algorithm>

Fade::Fade(Engine* engine)
{
	engine_ = engine;
}

void Fade::Initialize()
{
	spriteSize = { (float)kClientWidth,(float)kClientHeight };

	// デバッグ用のグローバル変数登録
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();
	gv->CreateGroup(groupName);

	gv->AddItem(groupName, "duration_", duration_);

	ApplyGlobalVariables();
}

void Fade::ApplyGlobalVariables()
{
	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();

	duration_ = gv->GetFloatValue(groupName, "duration_");
}

void Fade::Update()
{
	// フェード状態による分岐
	switch (status_)
	{
	case Status::None:
		// 何もしない
		break;
	case Status::FadeIn:
		// フェードイン
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;
		// フェード継続時間に達したら打ち止め
		if (counter_ >= duration_)
		{
			counter_ = duration_;
		}
		// 0.0fから1.0fの間で、経過時間がフェード継続時間に近づくほどアルファ値を大きくする
		color_.w = std::clamp(1.0f - counter_ / duration_, 0.0f, 1.0f);
		break;
	case Status::FadeOut:
		// フェードアウト
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;
		// フェード継続時間に達したら打ち止め
		if (counter_ >= duration_)
		{
			counter_ = duration_;
		}
		// 0.0fから1.0fの間で、経過時間がフェード継続時間に近づくほどアルファ値を大きくする
		color_.w = std::clamp(counter_ / duration_, 0.0f, 1.0f);
		break;
	}
}

void Fade::Draw()
{
	if (status_ == Status::None)
	{
		return;
	}
	engine_->renderer_->SubmitSprite(spritePos, spriteSize, 0.0f, Math::ColorVectorToUint32(color_), uvTransform, TextureHandle::Get(TextureID::white1x1));
}

void Fade::DebugDraw()
{
	ImGui::Begin("フェード");

	auto* gv = GlobalVariables::GetInstance();
	auto groupName = GetGlobalVariableGroupName();
	bool changed = false;

	if (ImGui::DragFloat("フェード時間（秒）", &duration_, 0.05f, 0.0f))
	{
		gv->SetValue(groupName, "duration_", duration_);
		changed = true;
	}

	if (changed)
	{
		ApplyGlobalVariables();
	}

	ImGui::End();
}

void Fade::Start(Status status, float duration)
{
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop()
{
	status_ = Status::None;
}

bool Fade::IsFinished() const
{
	// フェード状態による分岐
	switch (status_)
	{
	case Status::FadeIn:
	case Status::FadeOut:
		if (counter_ >= duration_)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}