# コードを書くときに必要なこと

このエンジンでゲームのコードを書くときに、守らないとビルドが通らない、または動かないことをまとめました。

## ファイルの書き方

- `.cpp` の **1 行目は必ず `#include "pch.h"`**。プリコンパイル済みヘッダーを使っているので、これがないとビルドエラーになる。標準ライブラリ・DirectX・ImGui・JSON などは `pch.h` で読み込み済みなので、個別に `#include` しなくてよい
- ヘッダーは `#pragma once` で始める
- 文字コードは UTF-8 で保存する。コードの中の日本語 (表示する文字列など) は UTF-8 として扱われるので、ほかの文字コードだと文字化けする

```cpp
#include "pch.h"      // 必ず最初
#include "Player.h"   // 自分のヘッダー
#include "GameInput.h"
```

## `#include` とフォルダ

`#include` は、フォルダ名を付けずにファイル名だけで書きます (`#include "Player.h"`)。
探す場所は「インクルードディレクトリ」に登録したフォルダだけなので、**新しいフォルダを作ったら、インクルードディレクトリにも足します。**

1. ソリューションエクスプローラーで `Game` を右クリック →「プロパティ」
2. 上の「構成」を「すべての構成」にする
3. 「C/C++」→「全般」→「追加のインクルードディレクトリ」の最後に、`;$(ProjectDir)Game\新しいフォルダ` を足す

ファイルを作ったら、ソリューションエクスプローラーで `Game` を右クリック →「追加」→「既存の項目」でプロジェクトに追加します。追加しないと `.cpp` がコンパイルされず、「未解決の外部シンボル」のエラーになります。

エンジンのクラスは、`Engine/` の中のフォルダに関係なく、ファイル名だけで `#include` できます (`#include "Model.h"` など)。

## 名前空間

- エンジンのものは、すべて `namespace FS` の中にある (`FS::GameObject`、`FS::Model` など)
- ヘッダーでは型に `FS::` を付ける。`.cpp` では `using namespace FS;` を書けば省略できる

```cpp
// Player.h
class Player : public FS::GameObject
{
    std::unique_ptr<FS::Model> model_;
};

// Player.cpp
using namespace FS;
```

ヘッダーに `using namespace FS;` を書くと、そのヘッダーを読み込んだすべてのファイルに効いてしまい、名前がぶつかったときに原因が分かりにくくなります。

## 持ち方

| もの | 持ち方 |
| :--- | :--- |
| 自分で作って持つもの (`Model`・`Sprite`・`Text`・`PropertyBinder` など) | `std::unique_ptr` |
| `objectManager_.Create` が返すオブジェクト | 生のポインタ。シーンが持っているので `delete` しない。消すときは `Destroy()` |
| `SpawnEmitter` が返すエミッター | 生のポインタ。`ParticleSystem` が持っているので `delete` しない |
| `Engine*` | 生のポインタ。受け取って使うだけ |

## ゲームごとに決めるもの

ゲームによって中身が変わるものは、`Game/Global/` にまとめています。どれもゲーム側のファイルなので、自由に書き換えてかまいません。

| ファイル | 内容 |
| :--- | :--- |
| `Game/Global/GameDefine.h` | シーンの名前 (`SceneName`)、オブジェクトの種類 (`ObjectTag`)、更新の順番 (`ObjectUpdateOrder`) |
| `Game/Global/GameInput.h` / `.cpp` | 操作 (`GameAction`) とキー・ボタンの割り当て |
| `Game/Global/CollisionConfig.h` | 当たり判定のレイヤー (`CollisionLayer`) |

`Engine/` の中は直接変えません。エンジンの更新で上書きされ、変えたところがぶつかります。
