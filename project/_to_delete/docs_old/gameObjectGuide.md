# FromSoheWare Engine GameObject ガイド

本ドキュメントでは、FromSoheWare エンジンにおけるゲームオブジェクトの基底クラス `FS::GameObject` と、それらを一括管理する `FS::GameObjectManager` の仕様および使い方について解説します。
エンジン内のキャラクター、アイテム、UIなどのあらゆる要素は、この `GameObject` を継承して作成し、マネージャーに登録することで動作します。

---

## 目次
- [1. GameObject の基本概念](#1-gameobject-の基本概念)
- [2. GameObjectManager による管理と生成](#2-gameobjectmanager-による管理と生成)
- [3. カスタムオブジェクトの作成（実装例）](#3-カスタムオブジェクトの作成実装例)
- [4. シーン (Scene) への組み込み](#4-シーン-scene-への組み込み)

---

## 1. GameObject の基本概念

`FS::GameObject` は、ゲーム内すべてのエンティティの基底クラスです。Transform（座標・回転・スケール）やライフサイクル、衝突判定のコールバック機能を備えています。

### 1-1. ライフサイクルメソッド
派生クラスで以下の仮想関数をオーバーライドして、独自の処理を実装します。

- `Initialize()` : オブジェクト生成時（またはシーン初期化時）に1度だけ呼ばれます。
- `Update()` : 毎フレーム呼ばれます。移動やロジックの更新を行います。
- `Draw()` : 毎フレームの描画パスで呼ばれます。モデルやスプライトの描画コマンドを発行します。
- `DebugDraw()` : デバッグ用の描画（コライダーの枠線の表示など）を行います。

### 1-2. トランスフォームの管理
オブジェクトの空間情報は `WorldTransform` によって管理されています。
`GetTransform()` を通じて、位置 (`translation_`)、回転 (`rotationQuaternion_`)、スケール (`scale_`) にアクセスします。行列の更新はマネージャー側で自動で行われます。

### 1-3. 状態管理と破棄
- **アクティブ状態** : `SetActive(bool)` で更新・描画を一時的にオン/オフできます（`IsActive()` で判定）。
- **破棄 (Destroy)** : `Destroy()` を呼ぶと内部の `isDead_` フラグが立ち、そのフレームの終わりにマネージャーから安全に削除されます。

### 1-4. タグ (Tag) と優先度
- **タグ** : `SetTag(uint32_t)` でタグを設定し、マネージャー経由で特定のオブジェクトを検索する際に使用します。
- **更新優先度** : `SetUpdatePriority(int)` を設定することで、`Update()` が呼ばれる順番を制御できます（値が小さいほど先に呼ばれる）。

---

## 2. GameObjectManager による管理と生成

シーン内のすべての `GameObject` は `FS::GameObjectManager` によって管理されます。マネージャーはオブジェクトの初期化、ソート、更新、描画、破棄を一括で行います。

### 2-1. オブジェクトの生成と登録 (`Create<T>`)
手動で `new` するのではなく、マネージャーの `Create<T>()` テンプレート関数を使用します。これにより、生成と同時にマネージャーへの登録が完了します。
コンストラクタに必要な引数は、そのまま `Create` の引数として渡すことができます。

```cpp
// 例: Player クラスが Player(Engine* engine, Camera* camera) というコンストラクタを持つ場合
auto player = objectManager_.Create<Player>(engine_, camera_);
```

### 2-2. オブジェクトの検索
設定したタグを用いて、他のオブジェクトを取得することができます。

```cpp
// タグが一致する最初のオブジェクトを取得
GameObject* target = objectManager_.FindObjectWithTag(ObjectTag::Player);

// タグが一致するすべてのオブジェクトをリストで取得
std::vector<GameObject*> enemies = objectManager_.FindObjectsWithTag(ObjectTag::Enemy);
```
### 2-3. 追加と削除のタイミング
- `Update()` の実行中に `Create<T>()` で生成したオブジェクトは、いったん待機リストに入り、**そのフレームの全オブジェクトの更新が終わった後**に `Initialize()` が呼ばれて本リストへ追加されます。弾の発射など、更新中にオブジェクトを増やしても安全です。
- `Destroy()` されたオブジェクトは、全オブジェクトの更新が終わった後にまとめて削除されます。
- `SetActive(false)` や `Destroy()` 済みのオブジェクトは、`Update()`・`Draw()`・`DebugDraw()` が呼ばれず、`FindObjectWithTag()` の検索対象からも外れます。
- 全オブジェクトの `Update()` の直後に、マネージャーが各オブジェクトの `GetTransform().UpdateMatrix()` を呼んでワールド行列を更新します。

---

## 3. カスタムオブジェクトの作成（実装例）

ここでは、その場で回転し、プレイヤーに触れると消える「コイン」を例に、`GameObject` を継承したクラスの作り方を説明します。

### Step 1: ヘッダー (`Coin.h`)

```cpp
#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "PropertyBinder.h"

class Coin : public FS::GameObject
{
public:
    Coin(FS::Engine* engine, const FS::Vector3& position);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // 衝突した瞬間に呼ばれる
    void OnCollisionEnter(FS::Collider* mine, FS::Collider* other) override;

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::Model> model_;
    std::unique_ptr<FS::Collider> collider_;
    std::unique_ptr<FS::PropertyBinder> binder_;

    float rotateSpeed_ = 1.0f;  // 1 秒あたりの回転量 (ラジアン)
    float angle_ = 0.0f;
};
```

### Step 2: 実装 (`Coin.cpp`)

```cpp
#include "pch.h"
#include "Coin.h"
#include "GameDefine.h"
#include "CollisionConfig.h"
#include "TimeManager.h"

using namespace FS;

Coin::Coin(Engine* engine, const Vector3& position)
    : engine_(engine)
{
    // タグと更新優先度
    SetTag(ObjectTag::Item);
    SetUpdatePriority(ObjectUpdateOrder::Default);

    // 初期位置
    GetTransform().translation_ = position;

    // ModelList.csv に登録した名前でモデルを生成
    model_ = std::make_unique<Model>(engine_, "sphere");

    // コライダーと PropertyBinder を生成 (グループ名 "Coin" → Coin.json に保存される)
    collider_ = std::make_unique<Collider>(this);
    binder_ = std::make_unique<PropertyBinder>(engine_, "Coin");
}

void Coin::Initialize()
{
    // 調整したいパラメータを登録 (保存済みの値があれば読み込まれる)
    binder_->Bind("RotateSpeed", &rotateSpeed_, 1.0f, 0.01f, 0.0f, 10.0f);

    // コライダーの設定
    collider_->SetType(CollisionShapeType::Sphere);
    collider_->SetRadius(0.5f);
    collider_->SetCollisionAttribute(kCollisionAttributeItem);
    collider_->SetCollisionMask(kCollisionAttributePlayer);

    // マネージャーに登録 (GameObjectManager に追加された後、つまり Initialize() 以降で呼ぶ)
    collider_->RegisterToManager();
}

void Coin::Update()
{
    float dt = TimeManager::GetInstance()->GetDeltaTime();

    // Y 軸回転
    angle_ += rotateSpeed_ * dt;
    GetTransform().rotationQuaternion_ = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, angle_);

    // モデルの位置・回転をオブジェクトに合わせる
    model_->GetTransform().translation_ = GetTransform().translation_;
    model_->GetTransform().rotationQuaternion_ = GetTransform().rotationQuaternion_;
}

void Coin::Draw()
{
    model_->Draw();
}

void Coin::DebugDraw()
{
    collider_->DrawCollider();

#ifdef ENABLE_IMGUI
    if (ImGui::Begin("Coin"))
    {
        binder_->Draw("RotateSpeed", "回転速度");
    }
    ImGui::End();
#endif
}

void Coin::OnCollisionEnter(Collider* mine, Collider* other)
{
    if (other->GetOwner()->CompareTag(ObjectTag::Player))
    {
        Destroy(); // このフレームの更新後に削除される
    }
}
```

### Step 3: 衝突属性の追加

衝突属性は `Game/Global/CollisionConfig.h` にビットフラグとして定義します。

```cpp
const uint32_t kCollisionAttributePlayer = 0b1;
const uint32_t kCollisionAttributeItem   = 0b1 << 3; // 追加
const uint32_t kCollisionAttributeProp   = 0b1 << 2;
const uint32_t kCollisionAttributeEnemy  = 0b1 << 10;
```

:::{important}
`CollisionManager` は、**お互いの属性が相手のマスクに含まれている場合にだけ**衝突を判定します (`A の属性 & B のマスク` と `B の属性 & A のマスク` の両方が 0 でないとき)。
上の例でコインとプレイヤーを衝突させるには、コイン側のマスクに `kCollisionAttributePlayer` を入れるだけでなく、`Player` 側のマスクにも `kCollisionAttributeItem` を追加する必要があります。
:::

### 3-1. Collider の主な設定

| 関数 | 内容 |
| :--- | :--- |
| `SetType(CollisionShapeType)` | 形状。`Sphere` (球) または `AABB` (軸平行ボックス) |
| `SetRadius(float)` | 球の半径 (既定値 1.0) |
| `SetSize(const Vector3&)` | AABB の **半分の** サイズ (既定値 0.5, 0.5, 0.5) |
| `SetCenterOffset(const Vector3&)` | オブジェクトの位置からのずれ。`SetApplyRotation(true)` (既定) ならオブジェクトの回転も適用される |
| `SetCollisionAttribute(uint32_t)` | 自分の属性 (既定値 0xffffffff = すべて) |
| `SetCollisionMask(uint32_t)` | 衝突相手として受け付ける属性 (既定値 0xffffffff = すべて) |
| `SetEnable(bool)` | コライダー単体の有効/無効 |
| `RegisterToManager()` | 所属するシーンの `CollisionManager` に登録する |
| `DrawCollider()` | コライダーの形をデバッグ表示する |

衝突すると、コライダーの持ち主の `OnCollisionEnter` (衝突した瞬間)、`OnCollisionStay` (衝突中の毎フレーム)、`OnCollisionExit` (離れた瞬間) が呼ばれます。`Collider` は破棄されると自動でマネージャーから登録解除されます。

### 3-2. レイキャスト

シーンの `CollisionManager` を使って、登録済みの全コライダーに対するレイキャストができます。

```cpp
FS::RaycastHit hit;
auto* collisionManager = GetManager()->GetCollisionManager();

if (collisionManager->Raycast(origin, direction, 100.0f, &hit, kCollisionAttributeEnemy))
{
    // hit.hitObject   : 当たったオブジェクト
    // hit.point       : 当たった位置 (ワールド座標)
    // hit.normal      : 当たった面の法線
    // hit.distance    : 始点からの距離
}
```

---

## 4. シーン (Scene) への組み込み

作成したオブジェクトは、シーンのコンストラクタで `objectManager_.Create<T>()` を呼んで登録します。登録したオブジェクトの `Initialize()` → `Update()` → `Draw()` → `DebugDraw()` は、シーンから自動で呼ばれます。

```cpp
PlayScene::PlayScene(Engine* engine)
    : BaseScene(engine, "PlayScene")
{
    auto player = objectManager_.Create<Player>(engine_, camera_.get());

    // コインを 3 つ配置
    objectManager_.Create<Coin>(engine_, Vector3{ 0.0f, 1.0f, 5.0f });
    objectManager_.Create<Coin>(engine_, Vector3{ 2.0f, 1.0f, 5.0f });
    objectManager_.Create<Coin>(engine_, Vector3{ 4.0f, 1.0f, 5.0f });
}
```

:::{note}
シーンに登録したオブジェクトはシーンと同じ寿命を持ち、シーンを切り替えて戻ってきたときには **同じインスタンスに対して `Initialize()` がもう一度呼ばれます**。`Destroy()` したオブジェクトは戻りません。シーンに入るたびに状態をリセットしたい値は、コンストラクタではなく `Initialize()` で設定してください。
:::

シーン全体の作り方は [シーン構築ガイド](sceneGuide.md) を参照してください。
