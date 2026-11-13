# 1. シーンを作る

空と床だけがあるシーンを作ります。

## シーンの名前を決める

シーンは名前で区別します。名前は `Game/Global/GameDefine.h` の `SceneName` にまとめて書きます。`Sample` を足します。

```cpp
namespace SceneName
{
    inline const std::string Main = "MainScene";
    inline const std::string Sample = "SampleScene";   // 追加
}
```

## シーンのクラスを作る

`Game/Scene/` に `SampleScene.h` と `SampleScene.cpp` を作り、プロジェクトに追加します。
シーンは `FS::BaseScene` を継承し、必要な関数だけを上書きします。

```cpp
// Game/Scene/SampleScene.h
#pragma once
#include "BaseScene.h"

class SampleScene : public FS::BaseScene
{
public:
    SampleScene(FS::Engine* engine);

protected:
    void OnInitialize() override;
};
```

```cpp
// Game/Scene/SampleScene.cpp
#include "pch.h"
#include "SampleScene.h"
#include "GameDefine.h"
#include "Environment.h"
#include "Grid.h"

using namespace FS;

SampleScene::SampleScene(Engine* engine)
    : BaseScene(engine, SceneName::Sample)
{
    // シーンに置くものを作る。作ったものの更新・描画・破棄は、シーンがまとめて行う
    objectManager_.Create<Environment>(engine_);   // 空と太陽の光
    objectManager_.Create<Grid>(engine_);          // 床の目安になる格子
}

void SampleScene::OnInitialize()
{
    // 斜め上から原点を見下ろす
    const Vector3 eye = { 0.0f, 12.0f, -14.0f };
    camera_->SetTranslation(eye);
    camera_->SetRotation(Quaternion::LookRotation(-eye, Vector3::Up()));
}
```

`Environment` と `Grid` は、エンジンが用意しているオブジェクトです。

| オブジェクト | 内容 |
| :--- | :--- |
| `Environment` | 空を描き、太陽の光と環境光を決める。**置かないと光がないので、モデルが真っ黒に映る** |
| `Grid` | 床の目安になる格子。Debug・Development でだけ見え、Release では描かれない |

ポイントは 2 つです。

- **コンストラクタ**: シーンに置くものを `objectManager_.Create<クラス名>(引数...)` で作る。ここで作ったものは、ゲームが終わるまで残る
- **`OnInitialize()`**: シーンに入るたびに呼ばれる。シーンに入り直したときにやり直したいこと (カメラの位置など) はここに書く

## シーンを登録する

`Game/Game.cpp` で、作ったシーンを登録します。

```cpp
#include "SampleScene.h"   // 追加

Game::Game()
    : Application(MakeConfig())
{
    // シーンを足したら、ここで登録する
    GetSceneManager().RegisterScene(std::make_unique<MainScene>(GetEngine()));
    GetSceneManager().RegisterScene(std::make_unique<SampleScene>(GetEngine()));   // 追加

    // 起動したときに最初に開くシーン。作っている間は、作っているシーンにしておくと楽
    GetSceneManager().SetInitialScene(SceneName::Sample);   // Main から変える
}
```

`Game` は、ウィンドウの設定 (`MakeConfig`) とシーンの登録だけを書く場所です。メインループや 1 フレームの処理の順番はエンジン (`FS::Application`) が受け持つので、ここに書く必要はありません。

## 動かしてみる

ビルドして起動すると、空と格子の床が映ります。
「シーンの選択」ウィンドウのボタンで、`MainScene` との切り替えもできます。

:::{tip}
カメラの位置は、シーン名のウィンドウ (「SampleScene」) の「メインカメラ設定」で動かせます。
「パラメータの保存」ウィンドウで保存すると、次の起動からはコードに書いた値ではなく保存した値が使われます。
:::

## 詳しく知りたいとき

- [シーン](../guides/scene.md): シーンの切り替え、シーンが持っているもの、呼ばれる順番
- [エフェクトとライト](../guides/effects.md#時間帯と天気): `Environment` の時間帯と天気
