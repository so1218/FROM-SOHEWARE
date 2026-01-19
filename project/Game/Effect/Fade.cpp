#include "Fade.h"
#include "TextureHandle.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"

#include <algorithm>

Fade::Fade(Engine* engine)
{
	engine_ = engine;

	sprite_ = std::make_unique<Sprite>(engine_);
}

void Fade::Initialize()
{
	spriteSize = { (float)kClientWidth,(float)kClientHeight };

	sprite_->SetPosition(spritePos);
	sprite_->SetSize(spriteSize);
	sprite_->SetTexture(TextureID::white1x1);

	sprite_->SetLayerOrder(9999);

	// デバッグ用のグローバル変数登録
	binder_ = std::make_unique<PropertyBinder>(std::vector<std::string>{"Fade"});

	binder_->Bind("duration_", &duration_, 1.0f, 0.05f);
}

void Fade::Update()
{
	// フェード状態に応じて処理
	switch (status_)
	{
	case Status::None:
		// フェードなし
		break;

	case Status::FadeIn:
		// フェードイン処理
		counter_ += 1.0f / 60.0f; // 1フレーム分を加算

		if (counter_ >= duration_)
		{
			counter_ = duration_;
		}

		// 経過に応じてアルファ値を0から1に
		color_.w = std::clamp(1.0f - counter_ / duration_, 0.0f, 1.0f);
		break;

	case Status::FadeOut:
		// フェードアウト処理
		counter_ += 1.0f / 60.0f; // 1フレーム分を加算

		if (counter_ >= duration_)
		{
			counter_ = duration_;
		}

		// 経過に応じてアルファ値を0から1に
		color_.w = std::clamp(counter_ / duration_, 0.0f, 1.0f);
		break;
	}

	// スプライトに反映
	sprite_->SetColor(Math::ColorVectorToUint32(color_));
}

void Fade::Draw()
{
	if (status_ == Status::None)
	{
		return;
	}
	sprite_->Draw();
}

void Fade::DebugDraw()
{
#ifdef IS_DEVELOPMENT
	ImGui::Begin("フェード");

	binder_->Draw("duration_", "フェード時間（秒）");

	ImGui::End();
#endif
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