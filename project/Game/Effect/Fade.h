#pragma once

#include "Engine.h"

class Fade
{
public:
	Fade(Engine* engine);

	void Initialize();
	void Update();
	void Draw();
	void DebugDraw();

	// 調整項目の適用
	void ApplyGlobalVariables();
	std::vector<std::string> GetGlobalVariableGroupName() const { return { "Fade" }; }

	Engine* engine_;

	Vector2 spritePos = { 0,0 };
	Vector2 spriteSize;
	Vector4 color_ = { 0,0,0,1 };

	WorldTransform uvTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

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
	float duration_ = 3.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;

	// フェード開始
	void Start(Status status, float duration);

	// フェード停止
	void Stop();

	// フェード終了判定
	bool IsFinished() const;
};