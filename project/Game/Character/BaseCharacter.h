#pragma once

#include <vector>

#include "Engine.h"
#include "Model.h"
#include "GameObject.h"

class BaseCharacter : public GameObject
{
public:
	virtual ~BaseCharacter() = default;

	// 初期化
	virtual void Initialize(Engine* engine);

	// 更新
	virtual void Update();

	// 描画
	virtual void Draw();

	// デバッグ描画処理
	virtual void DebugDraw();

protected:
	Engine* engine_ = nullptr;
};

