# FromSoheWare Engine 基本設計ガイド

本ドキュメントでは、FromSoheWare エンジン全体のアーキテクチャ、基本設計思想、および開発時に実装時に知っておくべき共通ルールについて解説します。

---

## 目次
- [1. エンジン構造と Engine* ポインタの伝搬ルール](#1-エンジン構造と-engine-ポインタの伝搬ルール)
- [2. プリコンパイル済みヘッダー (pch.h) の役割](#2-プリコンパイル済みヘッダー-pchh-の役割)
- [3. アプリケーションのライフサイクルと実行順序](#3-アプリケーションのライフサイクルと実行順序)
- [4. シングルトン機能との使い分け](#4-シングルトン機能との使い分け)

---

## 1. エンジン構造と Engine* ポインタを受け渡す流れ

FromSoheWare エンジンでは、過度なグローバル変数やシングルトン依存を避け、**「必要なコンポーネントに必要な依存関係を明示的に渡す」** 設計を採用しています。

### 1-1. なぜ Engine* を渡すのか？
描画処理、ライト管理、ポストエフェクト管理、プロパティバインダーの構築など、エンジンの基盤機能にアクセスするための起点となるのが `FS::Engine` クラスです。

生成される各オブジェクト（`BaseScene`、`GameObject`、`Player`、`Weapon` 等）は、コンストラクタ経由で `Engine*` を受け取り、内部に保持します。

```cpp
// 例：Player クラスのコンストラクタ
Player::Player(FS::Engine* engine, FS::Camera* camera)
    : engine_(engine), camera_(camera)
{
    SetTag(ObjectTag::Player);

    // Engine* を使用してコンポーネントやモデルを生成
    animationModel_ = std::make_unique<FS::AnimationModel>(engine_, "humanMesh", "humanRun");
    weapon_         = std::make_unique<FS::PlayerWeapon>(engine_);
    reticle_        = std::make_unique<FS::PlayerReticle>(engine_);

    // インゲーム調整用の PropertyBinder も Engine* を基に生成
    binder_   = std::make_unique<FS::PropertyBinder>(engine_, "Player");
    collider_ = std::make_unique<FS::Collider>(this);
}
```

### 1-2. Engine* ポインタを受け渡す流れ

`Engine*` は、以下のような順序で受け渡されていきます。

1. **Scene**
   `BaseScene` のコンストラクタで `Engine*` を受け取り、シーン内部で保持します。
2. **GameObject**
   `objectManager_.Create<T>(engine_, ...)` 経由でオブジェクトを生成する際、第1引数として `Engine*` を渡します。
3. **SubComponent**
   オブジェクトが所有する武器（`PlayerWeapon`）、UI（`PlayerReticle`）、モデル（`AnimationModel`）等も、親クラスから `Engine*` を引き継いで初期化します。

#### 実際のコード例

```cpp
// 1. シーン（PlayScene）で GameObject を生成する際、engine_ を渡す
auto player = objectManager_.Create<Player>(engine_, camera_.get());

// 2. GameObject（Player）のコンストラクタで受け取った engine を子コンポーネント群へ伝搬
Player::Player(FS::Engine* engine, FS::Camera* camera)
    : engine_(engine), camera_(camera)
{
    // 各サブコンポーネントの生成時に engine_ を渡す
    animationModel_ = std::make_unique<FS::AnimationModel>(engine_, "humanMesh", "humanRun");
    weapon_         = std::make_unique<FS::PlayerWeapon>(engine_);
    reticle_        = std::make_unique<FS::PlayerReticle>(engine_);
    binder_         = std::make_unique<FS::PropertyBinder>(engine_, "Player");
}