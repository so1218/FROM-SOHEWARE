#include "Fade.h"
#include "TextureHandle.h"

#include <algorithm>

Fade::Fade(Engine* engine)
{
	engine_ = engine;
}

void Fade::Initialize()
{
	spriteSize = { (float)kClientWidth,(float)kClientHeight };
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
	engine_->renderer_->DrawSprite(spritePos, spriteSize, 0.0f, ColorVectorToUint32(color_), uvTransform, TextureHandle::Get(TextureID::white1x1));
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