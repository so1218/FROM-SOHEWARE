# GameObject

シーンに置くもの (キャラクター・敵・アイテム・UI・マネージャー) は、すべて `FS::GameObject` を継承して作ります。
作ったオブジェクトはシーンの `objectManager_` に渡すだけで、更新・描画・後片付けをエンジンが行います。

## 作り方

```cpp
class Coin : public FS::GameObject
{
public:
    Coin(FS::Engine* engine, const FS::Vector3& position);

    void Initialize() override;   // シーンに入るたびに 1 回
    void Update() override;       // 毎フレーム
    void Draw() override;         // 毎フレーム
    void DebugGui() override;     // 毎フレーム (エディター画面のときだけ)
    void OnDestroy() override;    // Destroy() されて消える直前に 1 回
};
```

使わない関数は書かなくて構いません。シーンでは `Create` で作ります。引数はそのままコンストラクタに渡され、作ったオブジェクトのポインタが返ります。

```cpp
Coin* coin = objectManager_.Create<Coin>(engine_, Vector3{ 0.0f, 1.0f, 5.0f });
```

## 位置・回転・大きさ

オブジェクトは `GetTransform()` で位置などを持っています。モデルは別に位置を持っているので、`Update()` で合わせます。

```cpp
void Coin::Update()
{
    WorldTransform& transform = GetTransform();
    transform.translation_.y += 1.0f * deltaTime;                              // 位置
    transform.rotation_ = Quaternion::FromAxisAngle(Vector3::Up(), angle_);    // 回転 (クォータニオン)
    transform.scale_ = { 2.0f, 2.0f, 2.0f };                                   // 大きさ

    model_->GetTransform().translation_ = transform.translation_;
    model_->GetTransform().rotation_ = transform.rotation_;
}
```

回転を角度 (ラジアン) で指定したいときは `SetRotationEuler({ x, y, z })` を使います。

## ほかのオブジェクトを探す

まず、**作るときにポインタを渡す**方法を考えてください。依存関係がコードに出るので、読む人に分かりやすくなります。

```cpp
// シーンのコンストラクタ
Player* player = objectManager_.Create<Player>(engine_);
objectManager_.Create<Enemy>(engine_, player);   // 敵は作られたときからプレイヤーを知っている
```

後から探す必要があるときは、タグか型で探します。タグは `Game/Global/GameDefine.h` の `ObjectTag` にまとめます。

```cpp
// 自分のコンストラクタで
SetTag(ObjectTag::Enemy);

// ほかのオブジェクトから
GameObjectManager* manager = GetManager();
Player* player = manager->FindObjectWithTag<Player>(ObjectTag::Player);   // タグで 1 つ
std::vector<GameObject*> enemies = manager->FindObjectsWithTag(ObjectTag::Enemy);   // タグで全部
Player* player2 = manager->FindObject<Player>();   // 型で 1 つ
```

止めているもの・消したものは見つかりません。

## 止める・消す

| 関数 | どうなるか | 使う場面 |
| :--- | :--- | :--- |
| `SetActive(false)` | 更新・描画・当たり判定が止まる。`SetActive(true)` で戻る | 一時的に隠す。シーンをやり直したときに戻したいもの |
| `Destroy()` | そのフレームの更新が終わったあとで、シーンから完全に取り除かれる | 弾・倒した敵など、二度と使わないもの |

## 更新の順番

`SetUpdatePriority()` の値が小さいものから順に `Update()` が呼ばれます。値は `Game/Global/GameDefine.h` の `ObjectUpdateOrder` にまとめています。

```cpp
SetUpdatePriority(ObjectUpdateOrder::Player);   // 地形 (Field) のあと、ほかのもの (Default) の前
```

カメラがプレイヤーを追うなど「A の結果を B が使う」ときは、A を先にします。

## 状態ごとに処理を分ける

「待機・移動・攻撃」のように状態で動きが変わるものは、`StateMachine` を使うと 1 つの `Update()` が長くならずに済みます。

```cpp
class EnemyIdle : public State<Enemy>
{
public:
    void Enter(Enemy* enemy) override {}    // この状態に入ったとき
    void Update(Enemy* enemy) override
    {
        if (enemy->CanSeePlayer())
        {
            enemy->GetStateMachine()->ChangeState<EnemyChase>();   // 次の状態へ
        }
    }
    void Exit(Enemy* enemy) override {}     // この状態を抜けるとき
    const char* GetName() const override { return "Idle"; }
};

// Enemy.h (#include "StateMachine.h")
std::unique_ptr<StateMachine<Enemy>> stateMachine_;

// Enemy.cpp
stateMachine_ = std::make_unique<StateMachine<Enemy>>(this);   // コンストラクタで
stateMachine_->ChangeState<EnemyIdle>();                       // Initialize で (最初の状態)
stateMachine_->Update();                                       // Update で
```

状態の切り替えは、`ChangeState` を呼んだ時点ではなく、次に `Update()` を呼んだときに行われます。

`Player` が実際にこの方法で作られています (`Game/GameObject/Character/Player/PlayerState*.cpp`)。

## つまずきやすい点

- **コンストラクタと `Initialize()` の使い分け。** コンストラクタはゲームを通して 1 回、`Initialize()` はシーンに入るたびに呼ばれる。モデルの読み込みなど 1 回でよいものはコンストラクタ、位置や HP の初期化は `Initialize()` に書く
- **`Update()` の中で作ったオブジェクトは、そのフレームの終わりに追加される。** 弾を撃つなどで `Create` しても安全だが、作ったその場では `Initialize()` はまだ呼ばれていない
- **`Destroy()` したオブジェクトのポインタを持ち続けない。** 次のフレームには消えている。ほかから参照されるものは `SetActive(false)` で止めるか、`OnDestroy()` で参照している側に知らせる
