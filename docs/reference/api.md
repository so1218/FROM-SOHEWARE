# API リファレンス

ゲームを作るときに使うクラスの、すべての関数の宣言です。C++ のヘッダーから Doxygen で自動生成しています。
使い方はここではなく [機能別ガイド](../guides/index.md) と [早見表](cheatsheet.md) にまとめています。

:::{note}
エンジン内部だけで使うクラス (レンダラー・リソース管理など) は載せていません。それらはヘッダーを直接見てください。
また、ヘッダーのコメントは `//` で書いているため説明文は表示されず、宣言だけが表示されます。説明を載せたい関数は `///` でコメントを書くと、次のビルドから表示されます ([このドキュメントのビルド方法](building-docs.md#api-リファレンスに説明文を載せるには))。
:::

## シーンとオブジェクト

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Application <api-fs-application>` | メインループ。ゲームは継承してシーンを登録する |
| {ref}`FS::BaseScene <api-fs-basescene>` | シーンの基底クラス。派生して OnInitialize などを上書きする |
| {ref}`FS::SceneManager <api-fs-scenemanager>` | シーンの登録と切り替え |
| {ref}`FS::GameObject <api-fs-gameobject>` | シーンに置くものの基底クラス |
| {ref}`FS::GameObjectManager <api-fs-gameobjectmanager>` | シーンに置いたものの作成・検索 |
| {ref}`State <api-state>` | 状態の基底クラス |
| {ref}`StateMachine <api-statemachine>` | 状態の切り替え |

(api-fs-application)=
### FS::Application

ヘッダー: `Engine/Framework/Application.h`

```{doxygenclass} FS::Application
:members:
:protected-members:
:undoc-members:
```

(api-fs-basescene)=
### FS::BaseScene

ヘッダー: `Engine/Framework/BaseScene.h`

```{doxygenclass} FS::BaseScene
:members:
:protected-members:
:undoc-members:
```

(api-fs-scenemanager)=
### FS::SceneManager

ヘッダー: `Engine/Framework/SceneManager.h`

```{doxygenclass} FS::SceneManager
:members:
:undoc-members:
```

(api-fs-gameobject)=
### FS::GameObject

ヘッダー: `Engine/Framework/GameObject.h`

```{doxygenclass} FS::GameObject
:members:
:undoc-members:
```

(api-fs-gameobjectmanager)=
### FS::GameObjectManager

ヘッダー: `Engine/Framework/GameObjectManager.h`

```{doxygenclass} FS::GameObjectManager
:members:
:undoc-members:
```

(api-state)=
### State

ヘッダー: `Engine/Framework/StateMachine.h`

```{doxygenclass} State
:members:
:undoc-members:
```

(api-statemachine)=
### StateMachine

ヘッダー: `Engine/Framework/StateMachine.h`

```{doxygenclass} StateMachine
:members:
:undoc-members:
```

## 描画

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Model <api-fs-model>` | 動かない 3D モデル |
| {ref}`FS::AnimationModel <api-fs-animationmodel>` | ボーンで動く 3D モデル |
| {ref}`FS::Sprite <api-fs-sprite>` | 画面に出す画像 |
| {ref}`FS::Text <api-fs-text>` | 画面に出す文字 |
| {ref}`FS::LightManager <api-fs-lightmanager>` | 太陽・点光源・スポットライト |
| {ref}`FS::ParticleSystem <api-fs-particlesystem>` | パーティクルのプリセットとエミッターの管理 |
| {ref}`FS::ParticleEmitter <api-fs-particleemitter>` | パーティクルの発生源 |

(api-fs-model)=
### FS::Model

ヘッダー: `Engine/Graphics/RenderSystem/Model.h`

```{doxygenclass} FS::Model
:members:
:undoc-members:
```

(api-fs-animationmodel)=
### FS::AnimationModel

ヘッダー: `Engine/Graphics/Animation/AnimationModel.h`

```{doxygenclass} FS::AnimationModel
:members:
:undoc-members:
```

(api-fs-sprite)=
### FS::Sprite

ヘッダー: `Engine/Graphics/RenderSystem/Sprite.h`

```{doxygenclass} FS::Sprite
:members:
:undoc-members:
```

(api-fs-text)=
### FS::Text

ヘッダー: `Engine/Graphics/UI/Text.h`

```{doxygenclass} FS::Text
:members:
:undoc-members:
```

(api-fs-lightmanager)=
### FS::LightManager

ヘッダー: `Engine/Graphics/RenderSystem/LightManager.h`

```{doxygenclass} FS::LightManager
:members:
:undoc-members:
```

(api-fs-particlesystem)=
### FS::ParticleSystem

ヘッダー: `Engine/Graphics/Particle/ParticleSystem.h`

```{doxygenclass} FS::ParticleSystem
:members:
:undoc-members:
```

(api-fs-particleemitter)=
### FS::ParticleEmitter

ヘッダー: `Engine/Graphics/Particle/ParticleEmitter.h`

```{doxygenclass} FS::ParticleEmitter
:members:
:undoc-members:
```

## カメラ

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Camera <api-fs-camera>` | シーンのカメラ |
| {ref}`FS::CameraManager <api-fs-cameramanager>` | カメラコントローラーの切り替えと画面の揺れ |
| {ref}`FS::ICameraController <api-fs-icameracontroller>` | カメラを動かす処理の基底クラス |

(api-fs-camera)=
### FS::Camera

ヘッダー: `Engine/Camera/Camera.h`

```{doxygenclass} FS::Camera
:members:
:undoc-members:
```

(api-fs-cameramanager)=
### FS::CameraManager

ヘッダー: `Engine/Camera/CameraManager.h`

```{doxygenclass} FS::CameraManager
:members:
:undoc-members:
```

(api-fs-icameracontroller)=
### FS::ICameraController

ヘッダー: `Engine/Camera/ICameraController.h`

```{doxygenclass} FS::ICameraController
:members:
:undoc-members:
```

## 当たり判定

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Collider <api-fs-collider>` | 当たり判定の形 |
| {ref}`FS::CollisionManager <api-fs-collisionmanager>` | 判定とレイキャスト |
| {ref}`FS::RaycastHit <api-fs-raycasthit>` | レイキャストの結果 |

(api-fs-collider)=
### FS::Collider

ヘッダー: `Engine/Collision/Collider.h`

```{doxygenclass} FS::Collider
:members:
:undoc-members:
```

(api-fs-collisionmanager)=
### FS::CollisionManager

ヘッダー: `Engine/Collision/CollisionManager.h`

```{doxygenclass} FS::CollisionManager
:members:
:undoc-members:
```

(api-fs-raycasthit)=
### FS::RaycastHit

ヘッダー: `Engine/Collision/CollisionManager.h`

```{doxygenstruct} FS::RaycastHit
:members:
:undoc-members:
```

## 入力・時間・音

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Input <api-fs-input>` | キーボード・マウス・パッド |
| {ref}`FS::InputBinding <api-fs-inputbinding>` | 操作に割り当てる入力 1 つ |
| {ref}`FS::RepeatInput <api-fs-repeatinput>` | 押しっぱなしでの繰り返し入力 |
| {ref}`FS::TimeManager <api-fs-timemanager>` | 経過時間・タイムスケール・一時停止 |
| {ref}`FS::AudioPlayer <api-fs-audioplayer>` | 効果音・BGM・3D サウンド |
| {ref}`FS::Sound3DSettings <api-fs-sound3dsettings>` | 3D サウンドの設定 |

(api-fs-input)=
### FS::Input

ヘッダー: `Engine/Core/Input.h`

```{doxygenclass} FS::Input
:members:
:undoc-members:
```

(api-fs-inputbinding)=
### FS::InputBinding

ヘッダー: `Engine/Core/InputBinding.h`

```{doxygenclass} FS::InputBinding
:members:
:undoc-members:
```

(api-fs-repeatinput)=
### FS::RepeatInput

ヘッダー: `Engine/Core/RepeatInput.h`

```{doxygenclass} FS::RepeatInput
:members:
:undoc-members:
```

(api-fs-timemanager)=
### FS::TimeManager

ヘッダー: `Engine/Core/TimeManager.h`

```{doxygenclass} FS::TimeManager
:members:
:undoc-members:
```

(api-fs-audioplayer)=
### FS::AudioPlayer

ヘッダー: `Engine/Audio/AudioPlayer.h`

```{doxygenclass} FS::AudioPlayer
:members:
:undoc-members:
```

(api-fs-sound3dsettings)=
### FS::Sound3DSettings

ヘッダー: `Engine/Audio/AudioPlayer.h`

```{doxygenstruct} FS::Sound3DSettings
:members:
:undoc-members:
```

## 調整とデバッグ

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::PropertyBinder <api-fs-propertybinder>` | 変数の調整・保存・読み込み |
| {ref}`FS::DebugDraw <api-fs-debugdraw>` | 線や形のデバッグ表示 |

(api-fs-propertybinder)=
### FS::PropertyBinder

ヘッダー: `Engine/Debugs/PropertyBinder.h`

```{doxygenclass} FS::PropertyBinder
:members:
:undoc-members:
```

(api-fs-debugdraw)=
### FS::DebugDraw

ヘッダー: `Engine/Debugs/DebugDraw.h`

```{doxygenclass} FS::DebugDraw
:members:
:undoc-members:
```

## 数学

| 名前 | 内容 |
| :--- | :--- |
| {ref}`FS::Vector2 <api-fs-vector2>` |  |
| {ref}`FS::Vector3 <api-fs-vector3>` |  |
| {ref}`FS::Vector4 <api-fs-vector4>` |  |
| {ref}`FS::Quaternion <api-fs-quaternion>` | 回転 |
| {ref}`FS::WorldTransform <api-fs-worldtransform>` | 位置・回転・大きさと親子関係 |
| {ref}`FS::Easing <api-fs-easing>` | イージング関数 |

(api-fs-vector2)=
### FS::Vector2

ヘッダー: `Engine/Math/Vector/Vector2.h`

```{doxygenclass} FS::Vector2
:members:
:undoc-members:
```

(api-fs-vector3)=
### FS::Vector3

ヘッダー: `Engine/Math/Vector/Vector3.h`

```{doxygenclass} FS::Vector3
:members:
:undoc-members:
```

(api-fs-vector4)=
### FS::Vector4

ヘッダー: `Engine/Math/Vector/Vector4.h`

```{doxygenclass} FS::Vector4
:members:
:undoc-members:
```

(api-fs-quaternion)=
### FS::Quaternion

ヘッダー: `Engine/Math/Quaternion.h`

```{doxygenclass} FS::Quaternion
:members:
:undoc-members:
```

(api-fs-worldtransform)=
### FS::WorldTransform

ヘッダー: `Engine/Math/WorldTransform.h`

```{doxygenclass} FS::WorldTransform
:members:
:undoc-members:
```

(api-fs-easing)=
### FS::Easing

ヘッダー: `Engine/Math/Easing.h`

```{doxygenclass} FS::Easing
:members:
:undoc-members:
```

## 名前空間の関数

(api-gameinput)=
### GameInput

ゲームの操作の読み取り (ゲーム側)。ヘッダー: `Game/Global/GameInput.h`

```{doxygennamespace} GameInput
:members:
:undoc-members:
```

(api-fs-deverror)=
### FS::DevError

開発中に直すべき間違いの報告。ヘッダー: `Engine/Debugs/DevError.h`

```{doxygennamespace} FS::DevError
:members:
:undoc-members:
```

(api-fs-math)=
### FS::Math

数学の便利関数。ヘッダー: `Engine/Math/Utility/MathUtils.h`

```{doxygennamespace} FS::Math
:members:
:undoc-members:
```
