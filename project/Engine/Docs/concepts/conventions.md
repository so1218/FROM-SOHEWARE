# コードの決まり

チームで同じ書き方にそろえるための決まりです。迷ったら、近くにある既存のコードに合わせてください。

## ファイルの書き方

- `.cpp` の **1 行目は必ず `#include "pch.h"`**。標準ライブラリ・DirectX・ImGui・JSON などはここで読み込み済みなので、個別に `#include` しなくてよい
- ヘッダーは `#pragma once` で始める
- 文字コードは UTF-8 (BOM なし)、改行は CRLF

```cpp
#include "pch.h"      // 必ず最初
#include "Player.h"   // 自分のヘッダー
#include "GameInput.h"
```

## 名前空間

- エンジンのものはすべて `namespace FS` の中にある (`FS::GameObject`、`FS::Model` など)
- ゲームのもの (`Game/`) は名前空間なしで書く
- **ヘッダーでは `using namespace` を書かない。** 型には `FS::` を付ける。`.cpp` では `using namespace FS;` を書いてよい

```cpp
// Player.h
class Player : public FS::GameObject
{
    std::unique_ptr<FS::Model> model_;
};

// Player.cpp
using namespace FS;
```

## 名前の付け方

| 対象 | 書き方 | 例 |
| :--- | :--- | :--- |
| クラス・関数 | 大文字で始める | `PlayerWeapon`、`StartReload()` |
| 変数・引数 | 小文字で始める | `moveSpeed`、`targetPosition` |
| メンバー変数 | 小文字で始め、最後に `_` | `moveSpeed_` |
| 定数 | `k` で始める | `kMaxHp` |
| bool | `is` / `has` / `can` で始める | `isVisible_`、`canJump` |
| PropertyBinder のキー | 大文字で始める | `"MoveSpeed"` |

- 名前は略さない (`pos` ではなく `position`、`dt` ではなく `deltaTime`)。ループの `i` / `j` は例外
- 単位がまぎらわしい値は、名前かコメントに単位を書く (`durationSeconds`、`// ラジアン`)

## コメント

- コメントは日本語で、**コードを読んでも分からないこと** (なぜそうするのか、単位、注意点) を書く
- コードをそのまま言い換えただけのコメントは書かない

```cpp
// よくない: コードを読めば分かる
// 速度に時間を掛けて位置に足す
position += velocity * deltaTime;

// よい: なぜそうするのかが分かる
// 着地した瞬間に次のジャンプを受け付けると二段ジャンプになるので、1 フレーム待つ
canJump_ = wasGrounded_ && isGrounded_;
```

## 持ち方

- 自分が作って持つもの (モデル・部品) は `std::unique_ptr` で持つ
- 人のものを使うだけのもの (`Engine*`、ほかの GameObject、エミッター) は生のポインタで持つ。`delete` しない
- `objectManager_.Create` が返すポインタはシーンが持っている。消すときは `Destroy()` を呼ぶ

## エンジンとゲームの分け方

- `Engine/` には、どのゲームでも使えるものだけを書く。`Engine/` から `Game/` のファイルを `#include` しない
- このゲームだけのもの (操作の割り当て、レイヤー、タグ、シーン名) は `Game/Global/` にまとめる

| ファイル | 内容 |
| :--- | :--- |
| `Game/Global/GameDefine.h` | シーン名 (`SceneName`)、タグ (`ObjectTag`)、更新の順番 (`ObjectUpdateOrder`) |
| `Game/Global/GameInput.h/.cpp` | 操作 (`GameAction`) とキー・ボタンの割り当て |
| `Game/Global/CollisionConfig.h` | 当たり判定のレイヤー (`CollisionLayer`) |

## 新しいファイルとフォルダを追加する

1. ファイルを作り、Visual Studio のソリューションエクスプローラーで右クリック →「追加」→「既存の項目」でプロジェクトに追加する
2. **新しいフォルダを作ったときは、インクルードディレクトリにも足す。** プロジェクトのプロパティ →「C/C++」→「全般」→「追加のインクルードディレクトリ」に `$(ProjectDir)Game\新しいフォルダ` を足す。Debug・Development・Release の 3 つすべてに足すこと

`#include` はフォルダ名を付けずにファイル名だけで書く決まりなので (`#include "Player.h"`)、インクルードディレクトリに足していないフォルダのヘッダーは見つかりません。

エンジンのファイルは、ゲームのプロジェクトではなく共有プロジェクト `Engine/Engine.vcxitems` に入っています。エンジンにファイルを足すときは、ソリューションエクスプローラーの `Engine` に追加してください。エンジンに新しいフォルダを作ったときは、`Engine.vcxitems` の `AdditionalIncludeDirectories` に `$(MSBuildThisFileDirectory)新しいフォルダ` を足します (構成ごとに分かれていないので 1 か所でよい)。
