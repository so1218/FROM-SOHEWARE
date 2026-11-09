# 1. シーンを作る

空と床だけがある、空っぽのシーンを作ります。

## シーンの名前を決める

シーンは名前で区別します。名前は `Game/Global/GameDefine.h` の `SceneName` にまとめて書きます。

```cpp
namespace SceneName
{
    inline const std::string Play = "PlayScene";
    inline const std::string Test = "TestScene";
    inline const std::string Sample = "SampleScene";   // 追加
}
```

## シーンのクラスを作る

`Game/Scene/` に `SampleScene.h` と `SampleScene.cpp` を作ります。
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
    // シーンに置くものを作る。作ったものは、更新・描画・破棄をシーンがまとめて行う
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

ポイントは 2 つです。

- **コンストラクタ**: シーンに置くものを `objectManager_.Create<クラス名>(引数...)` で作る。ここで作ったものは、シーンが続く間ずっと残る
- **`OnInitialize()`**: シーンに入るたびに呼ばれる。シーンに入り直したときにやり直したいこと (カメラの位置など) はここに書く

## シーンを登録する

`Game/Game.cpp` で、作ったシーンを登録します。

```cpp
#include "SampleScene.h"   // 追加

Game::Game()
    : Application(MakeConfig())
{
    SceneManager& sceneManager = GetSceneManager();

    sceneManager.RegisterScene(std::make_unique<PlayScene>(GetEngine()));
    sceneManager.RegisterScene(std::make_unique<TestScene>(GetEngine()));
    sceneManager.RegisterScene(std::make_unique<SampleScene>(GetEngine()));   // 追加

    // 起動したときに最初に開くシーン。作っている間はこれにしておくと楽
    sceneManager.SetInitialScene(SceneName::Sample);
}
```

`Game` は、ウィンドウの設定 (`MakeConfig`) とシーンの登録だけを書く場所です。メインループや 1 フレームの処理の順番はエンジン (`FS::Application`) が受け持つので、ここに書く必要はありません。

## 動かしてみる

ビルドして起動すると、空と格子の床だけが映ります。
「シーンの選択」ウィンドウのボタンで、ほかのシーンとの切り替えもできます。

:::{tip}
カメラの位置は、シーン名のウィンドウ (「SampleScene」) の「メインカメラ設定」で動かせます。
「パラメータの保存」で保存すると、次の起動からはコードに書いた値ではなく保存した値が使われます。
:::

## 詳しく知りたいとき

- [シーン](../guides/scene.md): シーンの切り替え、シーンが持っているもの、呼ばれる順番
