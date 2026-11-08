# 4. ジェムを拾う

ボールのまわりにジェムを並べ、ボールが触れたら消えてスコアが増えるようにします。

## 当たり判定の仕組み

当たり判定は、GameObject に「コライダー」(当たり判定の形) を付けて行います。

- コライダーは `AddSphereCollider(半径)` (球) か `AddBoxCollider(大きさ)` (箱) で付ける。登録と後片付けはエンジンが行う
- 2 つのコライダーが重なると、両方の GameObject の `OnCollisionEnter()` が呼ばれる
- 何と何が当たるかは「レイヤー」で決める。関係ないもの同士は判定しない

## レイヤーを足す

レイヤーは `Game/Global/CollisionConfig.h` にまとめて書きます。ジェム用の `Item` を足します。

```cpp
namespace CollisionLayer
{
    constexpr uint32_t Player = 1u << 1;
    constexpr uint32_t Enemy = 1u << 2;
    constexpr uint32_t Prop = 1u << 3;
    constexpr uint32_t Item = 1u << 4;   // 追加
}
```

## ジェムを作る

`Game/GameObject/Item/` に `Gem.h` と `Gem.cpp` を作ります。

```cpp
// Game/GameObject/Item/Gem.h
#pragma once
#include "GameObject.h"
#include "Model.h"

class Gem : public FS::GameObject
{
public:
    Gem(FS::Engine* engine, const FS::Vector3& position);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

private:
    std::unique_ptr<FS::Model> model_;
    float angle_ = 0.0f;
};
```

```cpp
// Game/GameObject/Item/Gem.cpp
#include "pch.h"
#include "Gem.h"
#include "Collider.h"
#include "CollisionConfig.h"
#include "GameDefine.h"
#include "TimeManager.h"

using namespace FS;

Gem::Gem(Engine* engine, const Vector3& position)
{
    // ほかのオブジェクトから「アイテム」だと分かるようにする
    SetTag(ObjectTag::Item);
    GetTransform().translation_ = position;

    model_ = std::make_unique<Model>(engine, "cube");
    model_->GetTransform().scale_ = { 0.5f, 0.5f, 0.5f };
    model_->SetColor(Vector4{ 0.2f, 0.8f, 1.0f, 1.0f });

    // 自分は Item で、Player とだけ当たる
    Collider* collider = AddSphereCollider(0.8f);
    collider->SetCollisionLayer(CollisionLayer::Item);
    collider->SetCollisionMask(CollisionLayer::Player);
}

void Gem::Initialize()
{
    // シーンに入り直したら、拾われたジェムも元に戻す
    SetActive(true);
}

void Gem::Update()
{
    angle_ += 2.0f * TimeManager::GetInstance()->GetDeltaTime();

    model_->GetTransform().translation_ = GetTransform().translation_;
    model_->GetTransform().rotation_ = Quaternion::FromAxisAngle(Vector3::Up(), angle_);
}

void Gem::Draw()
{
    model_->Draw();
}

void Gem::OnCollisionEnter(Collider* mine, Collider* other)
{
    // 拾われたら止める。止めている間は、更新・描画・当たり判定がすべて行われない
    SetActive(false);
}
```

:::{note}
消すだけなら `Destroy()` もあります。`Destroy()` したオブジェクトはシーンから完全に取り除かれ、シーンに入り直しても戻りません。
シーンをやり直したときに元に戻したいものは、このように `SetActive(false)` で止めておき、`Initialize()` で `SetActive(true)` に戻します。
:::

## ボールにもコライダーを付けて、スコアを数える

`Ball.h` に、衝突したときの関数とスコアを足します。

```cpp
public:
    // ...
    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;   // 追加
    int GetScore() const { return score_; }                                    // 追加

private:
    // ...
    int score_ = 0;   // 追加
```

`Ball.cpp` を次のように変えます。

```cpp
#include "Collider.h"          // 追加
#include "CollisionConfig.h"   // 追加
#include "GameDefine.h"        // 追加

Ball::Ball(Engine* engine) : engine_(engine)
{
    model_ = std::make_unique<Model>(engine_, "sphere");
    binder_ = std::make_unique<PropertyBinder>(engine_, "Ball");

    // 自分は Player で、Item とだけ当たる
    Collider* collider = AddSphereCollider(1.0f);
    collider->SetCollisionLayer(CollisionLayer::Player);
    collider->SetCollisionMask(CollisionLayer::Item);
}

void Ball::Initialize()
{
    GetTransform().translation_ = { 0.0f, 1.0f, 0.0f };
    score_ = 0;
    binder_->Bind("MoveSpeed", &moveSpeed_, 6.0f, 0.1f, 0.0f, 50.0f);
}

void Ball::OnCollisionEnter(Collider* mine, Collider* other)
{
    // other は相手のコライダー。GetOwner() で相手の GameObject が分かる
    if (other->GetOwner()->CompareTag(ObjectTag::Item))
    {
        ++score_;
    }
}
```

:::{important}
当たるのは「お互いのマスクに、相手のレイヤーが入っているとき」だけです。
ジェム側に `SetCollisionMask(CollisionLayer::Player)` を書いても、ボール側のマスクに `CollisionLayer::Item` がなければ当たりません。
:::

## シーンにジェムを並べる

`SampleScene.cpp` のコンストラクタで、ボールのまわりに 8 個並べます。

```cpp
#include "Gem.h"         // 追加
#include "MathUtils.h"   // 追加

SampleScene::SampleScene(Engine* engine)
    : BaseScene(engine, SceneName::Sample)
{
    objectManager_.Create<Environment>(engine_);
    objectManager_.Create<Grid>(engine_);
    objectManager_.Create<Ball>(engine_);

    // 半径 6 の円の上に等間隔に置く
    for (int i = 0; i < 8; ++i)
    {
        const float angle = Math::PI * 2.0f * static_cast<float>(i) / 8.0f;
        objectManager_.Create<Gem>(engine_, Vector3{ std::cos(angle) * 6.0f, 1.0f, std::sin(angle) * 6.0f });
    }
}
```

## 動かしてみる

ボールでジェムに触れると、ジェムが消えます。
当たり判定の形はエディター画面で水色の線で表示されます。見えないときは、シーン名のウィンドウの「コライダーを表示」を確認してください。

## 詳しく知りたいとき

- [当たり判定](../guides/collision.md): 箱のコライダー、位置のずらし方、レイキャスト
- [GameObject](../guides/gameobject.md): タグ、オブジェクトの探し方、`SetActive` と `Destroy` の違い
