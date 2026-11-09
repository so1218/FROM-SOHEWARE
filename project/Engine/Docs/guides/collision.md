# 当たり判定

GameObject に「コライダー」(当たり判定の形) を付けると、ほかのコライダーと重なったときに知らせが届きます。
レイ (線) を飛ばして、最初に当たったものを調べることもできます。

## コライダーを付ける

```cpp
// コンストラクタで。形を決めて付ける。登録と後片付けはエンジンが行う
Collider* collider = AddSphereCollider(0.5f);             // 球 (半径)
Collider* body = AddBoxCollider({ 1.0f, 2.0f, 1.0f });    // 箱 (幅・高さ・奥行き)
```

コライダーはオブジェクトの位置 (`GetTransform()`) に付いていきます。1 つのオブジェクトに、いくつでも付けられます。

| 関数 | 内容 |
| :--- | :--- |
| `SetRadius(float)` / `SetSize(Vector3)` | 大きさを変える (球なら半径、箱なら幅・高さ・奥行き) |
| `SetCenterOffset(Vector3)` | オブジェクトの位置からずらす。足元が原点のキャラクターなら、高さの半分だけ上げる |
| `SetRotateCenterOffset(bool)` | ずらす向きをオブジェクトの回転に合わせるか (既定は合わせる)。箱そのものは回らない |
| `SetCollisionLayer(uint32_t)` / `SetCollisionMask(uint32_t)` | 何と当たるか (下で説明) |
| `SetEnable(bool)` | 一時的に無効にする |
| `SetRaycastOnly(bool)` | ほかのコライダーとは判定せず、レイだけに当たる。弱点など |

:::{note}
箱は回転しません。いつもワールドの X・Y・Z の向きに沿った箱です。回るものには球を使うか、少し大きめの箱で包んでください。
:::

## 当たったときの処理

重なったときに、両方の GameObject の関数が呼ばれます。`mine` は自分のコライダー、`other` は相手のコライダーです。

```cpp
void Enemy::OnCollisionEnter(Collider* mine, Collider* other)   // 重なった瞬間
{
    GameObject* hit = other->GetOwner();
    if (hit->CompareTag(ObjectTag::Player))
    {
        // プレイヤーに触れた
    }
}

void Enemy::OnCollisionStay(Collider* mine, Collider* other) {}   // 重なっている間、毎フレーム
void Enemy::OnCollisionExit(Collider* mine, Collider* other) {}   // 離れた瞬間
```

コライダーを複数付けたときは、`mine` を比べるとどれが当たったか分かります。

## 何と何が当たるか (レイヤー)

関係ないもの同士 (敵と敵など) を判定しないように、コライダーごとに「自分のレイヤー」と「当たる相手のレイヤー (マスク)」を決めます。
レイヤーは `Game/Global/CollisionConfig.h` にまとめて書きます。

```cpp
namespace CollisionLayer
{
    constexpr uint32_t Player = 1u << 1;
    constexpr uint32_t Enemy = 1u << 2;
    constexpr uint32_t Prop = 1u << 3;
    constexpr uint32_t Item = 1u << 4;   // 足すときは、使っていないビットを使う
}
```

```cpp
collider->SetCollisionLayer(CollisionLayer::Player);                          // 自分はプレイヤー
collider->SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::Item);     // 敵とアイテムに当たる
```

:::{important}
当たるのは「**お互いの**マスクに、相手のレイヤーが入っているとき」だけです。片方のマスクにだけ書いても当たりません。
:::

レイヤーを設定しないコライダーは、`kDefaultCollisionLayer` (1 ビット目) に属し、同じく設定していないものとだけ当たります。1 ビット目はゲームのレイヤーに使わないでください。

## レイを飛ばす

弾が当たったか、足元に何があるかなどは、レイで調べます。

```cpp
// #include "GameObjectManager.h" と "CollisionManager.h" が必要
CollisionManager* collisionManager = GetManager()->GetCollisionManager();

RaycastHit hit;
// 始点, 向き, 最大距離, 結果, 調べるレイヤー
if (collisionManager->Raycast(origin, direction, 100.0f, &hit, CollisionLayer::Enemy | CollisionLayer::Prop))
{
    hit.hitObject;     // 当たった GameObject
    hit.hitCollider;   // 当たったコライダー
    hit.point;         // 当たった位置
    hit.normal;        // 当たった面の向き
    hit.distance;      // 始点からの距離
}
```

レイは最後の引数に入っているレイヤーのコライダーだけを調べます (マスクは見ません)。

## 押し戻す

壁にめり込まないようにするには、`OnCollisionStay` で押し戻す量を計算して位置を動かします。

```cpp
void Wall::OnCollisionStay(Collider* mine, Collider* other)
{
    Vector3 push;
    // mine を基準に、other を外に出すための移動量
    if (mine->CalculatePushBackVector(other, push))
    {
        other->GetOwner()->GetTransform().translation_ += push;
    }
}
```

実例は `Game/GameObject/Field/EnvironmentProp.cpp` (乗れる足場) です。

## 確認する

エディター画面では、コライダーの形が線で表示されます。シーン名のウィンドウの「コライダーを表示」で ON / OFF できます。線の色は `SetColor()` で変えられます。

## つまずきやすい点

- **当たらない。** まず「コライダーを表示」で形と位置を確認する。次に、両方のマスクに相手のレイヤーが入っているか確認する
- **当たり判定は、全オブジェクトの `Update()` のあと、`Draw()` の前に行われる。** `OnCollisionEnter` で `Destroy()` したものは、そのフレームから描かれない
- **止めたオブジェクト (`SetActive(false)`) は当たらない。** 消すつもりがないなら、コライダーだけを `SetEnable(false)` にする方法もある
