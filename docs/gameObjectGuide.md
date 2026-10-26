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