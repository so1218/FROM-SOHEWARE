#pragma once
#include "Engine.h"
#include "Sprite.h"
#include "PropertyBinder.h"

class Fade
{
public:
	Fade(Engine* engine);

	void Initialize();
	void Update();
	void Draw();
	void DebugDraw();

	float GetDuration() const { return duration_; }

	Engine* engine_;

	std::unique_ptr<Sprite> sprite_;

	Vector2 spritePos_ = { 0,0 };
	Vector2 spriteSize_;
	Vector4 color_ = { 0,0,0,1 };

	bool useAlphaFade_;    // 透明度変化を使うか
	bool useDissolve_;    // ディゾルブを使うか

	// パラメータ
	float dissolveThreshold_ = 0.0f;
	float edgeWidth_;
	float edgeIntensity_;
	Vector3 edgeColor_;
	float currentAlpha_ = 0.0f;

	// フェードの状態
	enum class Status
	{
		None, // フェードなし
		FadeIn, // フェードイン中
		FadeOut // フェードアウト中
	};

	// 現在のフェード状態
	Status status_ = Status::None;

	// フェードの持続時間
	float duration_ = 1.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;

	// フェード開始
	void Start(Status status, float duration);

	// フェード停止
	void Stop();

	// フェード終了判定
	bool IsFinished() const;

	std::unique_ptr<PropertyBinder> binder_;
};