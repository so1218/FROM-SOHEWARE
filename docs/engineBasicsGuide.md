# FromSoheWare Engine 基本設計ガイド

本ドキュメントでは、FromSoheWare エンジン全体のアーキテクチャ、基本設計思想、および開発時に実装時に知っておくべき共通ルールについて解説します。

---

## 目次
- [1. エンジン構造と Engine* ポインタの伝搬ルール](#1-エンジン構造と-engine-ポインタの伝搬ルール)
- [2. プリコンパイル済みヘッダー (pch.h) の役割](#2-プリコンパイル済みヘッダー-pchh-の役割)
- [3. 名前空間 (namespace FS) とヘッダーの記述ルール](#3-名前空間-namespace-fs-とヘッダーの記述ルール)
- [4. アプリケーションのライフサイクルと実行順序](#4-アプリケーションのライフサイクルと実行順序)
- [5. シングルトン機能との使い分け](#5-シングルトン機能との使い分け)

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
```

## 2. プリコンパイル済みヘッダー (pch.h) の役割とルール

`pch.h` には、エンジン全体で頻繁に使用する主要な外部ライブラリや標準ヘッダーが一括で定義されています。これらを事前にまとめてコンパイルしておくことで、ビルド時間を大幅に短縮しています。

---

### 2-1. pch.h に含まれている主なライブラリ

以下のライブラリ群はすべて `pch.h` 内部で一度に読み込まれています。

- **C++ 標準ライブラリ**: `<vector>`, `<memory>`, `<string>`, `<algorithm>`, `<unordered_map>`, `<format>` など多数
- **Windows API & DirectX 12**: `<Windows.h>`, `<d3d12.h>`, `<dxgi1_6.h>`, `d3dx12.h`, `DirectXMath.h` など
- **入力・音声・メディア**: DirectInput, XInput, XAudio2, Media Foundation
- **プロファイリング & デバッグ**: PIX (`pix3.h`), Dear ImGui, ImGuizmo
- **外部ライブラリ**: nlohmann/json, DirectXTex, Assimp (`Importer.hpp` 等)

---

### 2-2. 開発時の必須ルール

1. **すべての `.cpp` ファイルの 1 行目に `#include "pch.h"` を書く**  
   新規に作成した `.cpp` ファイルも含め、コードの**最上段**には必ず `#include "pch.h"` を記述してください。

2. **`pch.h` に含まれるライブラリは個別インクルード不要**
   `std::vector` や `DirectX::XMFLOAT3`、`nlohmann::json` などを使う際、各 `.cpp` ファイルで個別に `<vector>` や `<d3d12.h>` を `#include` する必要はありません（`pch.h` を通して自動的に利用可能になります）。

```cpp
// 正しい書き方（必ず1行目に pch.h を書く）
#include "pch.h"
#include "Player.h"
#include "Enemy.h"

// 誤った書き方（pch.h より上に他のヘッダーがある）
#include "Player.h"
#include "pch.h" // プリコンパイルヘッダーの仕組み上、エラーになります
```

## 3. 名前空間 (namespace FS) とヘッダーの記述ルール

FromSoheWare エンジンの基盤機能は、ゲーム固有のコードや外部ライブラリとの名前の衝突を防ぐため、すべて **`namespace FS`** の中に定義されています。

---

### 3-1. FS 名前空間の構成
エンジンのコアクラス（`Engine`, `GameObject`, `Camera`, `AnimationModel`, `Terrain`, `Collider`, `PropertyBinder` など）は `FS` 名前空間に所属しています。

```cpp
// ヘッダー内での継承や型指定の例
class Player : public FS::GameObject
{
public:
    Player(FS::Engine* engine, FS::Camera* camera);
    
private:
    FS::Engine* engine_ = nullptr;
    FS::Camera* camera_ = nullptr;
    FS::Terrain* terrain_ = nullptr;
    std::unique_ptr<FS::AnimationModel> animationModel_;
    std::unique_ptr<FS::Collider> collider_;
};
```

### 3-2. ヘッダーファイル (.h) での `using namespace` の禁止

**「ヘッダーファイル内で `using namespace FS;` や `using namespace std;` を記述してはならない」** という規約を設けています。

#### なぜ禁止なのか？（名前空間汚染の防止）
ヘッダーファイル内で `using namespace` を使うと、そのヘッダーを `#include` した**すべてのファイル**にまで対象の空間が勝手に展開されてしまいます。
その結果、意図しない場所で同名の関数や型がぶつかり、原因の特定しづらいコンパイルエラーやバグを引き起こすためです。

#### 記述ルール
- **ヘッダーファイル (`.h`)**
  型名やクラス名には必ず `FS::` や `std::` を付与して明記します。
- **実装ファイル (`.cpp`)**
  `.cpp` ファイル内に限っては、ファイル冒頭で `using namespace FS;` や `using namespace std;` を記述してコードを簡略化しても問題ありません（他のファイルへ影響を与えないため）。

```cpp
// ヘッダーファイル (.h) での NG 例
#pragma once
#include "pch.h"

using namespace FS; // このヘッダーをインクルードした全ファイルに FS が展開されてしまう

class Player : public GameObject // FS:: が省略できてしまうがスコープが汚染される
{
    ...
};