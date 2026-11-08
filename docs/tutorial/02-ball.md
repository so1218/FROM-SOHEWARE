# 2. ボールを動かす

キーボード (WASD) とパッド (左スティック) で動くボールを作ります。

## GameObject を作る

シーンに置くもの (キャラクター・アイテム・UI など) は、すべて `FS::GameObject` を継承して作ります。
`Game/GameObject/` に `Ball.h` と `Ball.cpp` を作ります。

```cpp
// Game/GameObject/Ball.h
#pragma once
#include "GameObject.h"
#include "Model.h"

class Ball : public FS::GameObject
{
public:
    Ball(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Model> model_;

    float moveSpeed_ = 6.0f;   // 1 秒に進む距離
};
```

```cpp
// Game/GameObject/Ball.cpp
#include "pch.h"
#include "Ball.h"
#include "GameInput.h"
#include "TimeManager.h"

using namespace FS;

Ball::Ball(Engine* engine) : engine_(engine)
{
    // Assets/Models にある sphere.obj を使う。モデルはファイル名 (拡張子なし) で指定する
    model_ = std::make_unique<Model>(engine_, "sphere");
}

void Ball::Initialize()
{
    // シーンに入るたびに呼ばれる。最初の位置に戻す
    GetTransform().translation_ = { 0.0f, 1.0f, 0.0f };
}

void Ball::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 移動の入力。x が右、y が奥で、長さは 0 ~ 1
    const Vector2 move = GameInput::GetMoveAxis();
    GetTransform().translation_.x += move.x * moveSpeed_ * deltaTime;
    GetTransform().translation_.z += move.y * moveSpeed_ * deltaTime;

    // 見た目のモデルを、オブジェクトの位置に合わせる
    model_->GetTransform().translation_ = GetTransform().translation_;
}

void Ball::Draw()
{
    model_->Draw();
}
```

覚えておくことは 3 つです。

- **4 つの関数の役割**: `Initialize()` はシーンに入るたびに 1 回、`Update()` と `Draw()` は毎フレーム呼ばれる。中身はエンジンが呼ぶので、自分で呼ぶ必要はない
- **位置と見た目は別**: `GetTransform()` はオブジェクトの位置、`model_->GetTransform()` はモデルの位置。`Update()` でモデルをオブジェクトに合わせる
- **時間を掛ける**: 移動量に `GetDeltaTime()` (前のフレームからの秒数) を掛けると、FPS が変わっても同じ速さで動く

:::{note}
入力は `Input` (キーやボタンを直接見る) ではなく `GameInput` (「移動」「ジャンプ」などの操作で見る) を使います。
キーボードとパッドの両方に対応でき、割り当ての変更も 1 か所で済みます ([入力](../guides/input.md))。
:::

## シーンに置く

`SampleScene.cpp` のコンストラクタで、ボールを作ります。

```cpp
#include "Ball.h"   // 追加

SampleScene::SampleScene(Engine* engine)
    : BaseScene(engine, SceneName::Sample)
{
    objectManager_.Create<Environment>(engine_);
    objectManager_.Create<Grid>(engine_);
    objectManager_.Create<Ball>(engine_);   // 追加
}
```

`Create<Ball>(engine_)` の引数は、そのまま `Ball` のコンストラクタに渡されます。

## 動かしてみる

ビルドして起動し、WASD (パッドなら左スティック) でボールが動けば完成です。

## 詳しく知りたいとき

- [GameObject](../guides/gameobject.md): 呼ばれる順番、オブジェクトの探し方、消し方
- [3D モデルとアニメーション](../guides/model.md): 色・テクスチャの変更、アニメーションの再生
- [入力](../guides/input.md): 操作の追加、キーやボタンの割り当て
