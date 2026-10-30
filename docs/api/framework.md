# フレームワーク (Framework)

シーン・ゲームオブジェクト・シーン遷移・ステートマシンなど、ゲームを組み立てるための枠組みです。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::BaseScene <fs-basescene>` | class | `Engine/Framework/BaseScene.h` |  |
| {ref}`FS::TimeOfDayProfile <fs-timeofdayprofile>` | struct | `Engine/Framework/EnvironmentManager.h` |  |
| {ref}`FS::WeatherVisualParams <fs-weathervisualparams>` | struct | `Engine/Framework/EnvironmentManager.h` |  |
| {ref}`FS::WeatherProfile <fs-weatherprofile>` | struct | `Engine/Framework/EnvironmentManager.h` | 天候が環境に与える影響のプロファイル |
| {ref}`FS::EnvironmentManager <fs-environmentmanager>` | class | `Engine/Framework/EnvironmentManager.h` |  |
| {ref}`FS::Fade <fs-fade>` | class | `Engine/Framework/Fade.h` |  |
| {ref}`FS::FadeInState <fs-fadeinstate>` | class | `Engine/Framework/FadeInState.h` |  |
| {ref}`FS::FadeOutState <fs-fadeoutstate>` | class | `Engine/Framework/FadeOutState.h` |  |
| {ref}`FS::GameObject <fs-gameobject>` | class | `Engine/Framework/GameObject.h` |  |
| {ref}`FS::GameObjectManager <fs-gameobjectmanager>` | class | `Engine/Framework/GameObjectManager.h` |  |
| {ref}`FS::ISceneTransitionState <fs-iscenetransitionstate>` | class | `Engine/Framework/ISceneTransitionState.h` | 遷移状態の基底クラス |
| {ref}`FS::NormalState <fs-normalstate>` | class | `Engine/Framework/NormalState.h` |  |
| {ref}`FS::SceneManager <fs-scenemanager>` | class | `Engine/Framework/SceneManager.h` |  |
| {ref}`State <state>` | class | `Engine/Framework/StateMachine.h` | 状態の基底クラス |
| {ref}`StateMachine <statemachine>` | class | `Engine/Framework/StateMachine.h` | ステートを管理するマシン |
| {ref}`FS::TerrainChunk <fs-terrainchunk>` | class | `Engine/Framework/TerrainChunk.h` |  |

(fs-basescene)=
## FS::BaseScene

ヘッダー: `Engine/Framework/BaseScene.h`

```{doxygenclass} FS::BaseScene
:members:
:protected-members:
:undoc-members:
```

(fs-timeofdayprofile)=
## FS::TimeOfDayProfile

ヘッダー: `Engine/Framework/EnvironmentManager.h`

```{doxygenstruct} FS::TimeOfDayProfile
:members:
:protected-members:
:undoc-members:
```

(fs-weathervisualparams)=
## FS::WeatherVisualParams

ヘッダー: `Engine/Framework/EnvironmentManager.h`

```{doxygenstruct} FS::WeatherVisualParams
:members:
:protected-members:
:undoc-members:
```

(fs-weatherprofile)=
## FS::WeatherProfile

天候が環境に与える影響のプロファイル

ヘッダー: `Engine/Framework/EnvironmentManager.h`

```{doxygenstruct} FS::WeatherProfile
:members:
:protected-members:
:undoc-members:
```

(fs-environmentmanager)=
## FS::EnvironmentManager

ヘッダー: `Engine/Framework/EnvironmentManager.h`

```{doxygenclass} FS::EnvironmentManager
:members:
:protected-members:
:undoc-members:
```

(fs-fade)=
## FS::Fade

ヘッダー: `Engine/Framework/Fade.h`

```{doxygenclass} FS::Fade
:members:
:protected-members:
:undoc-members:
```

(fs-fadeinstate)=
## FS::FadeInState

ヘッダー: `Engine/Framework/FadeInState.h`

```{doxygenclass} FS::FadeInState
:members:
:protected-members:
:undoc-members:
```

(fs-fadeoutstate)=
## FS::FadeOutState

ヘッダー: `Engine/Framework/FadeOutState.h`

```{doxygenclass} FS::FadeOutState
:members:
:protected-members:
:undoc-members:
```

(fs-gameobject)=
## FS::GameObject

ヘッダー: `Engine/Framework/GameObject.h`

```{doxygenclass} FS::GameObject
:members:
:protected-members:
:undoc-members:
```

(fs-gameobjectmanager)=
## FS::GameObjectManager

ヘッダー: `Engine/Framework/GameObjectManager.h`

```{doxygenclass} FS::GameObjectManager
:members:
:protected-members:
:undoc-members:
```

(fs-iscenetransitionstate)=
## FS::ISceneTransitionState

遷移状態の基底クラス

ヘッダー: `Engine/Framework/ISceneTransitionState.h`

```{doxygenclass} FS::ISceneTransitionState
:members:
:protected-members:
:undoc-members:
```

(fs-normalstate)=
## FS::NormalState

ヘッダー: `Engine/Framework/NormalState.h`

```{doxygenclass} FS::NormalState
:members:
:protected-members:
:undoc-members:
```

(fs-scenemanager)=
## FS::SceneManager

ヘッダー: `Engine/Framework/SceneManager.h`

```{doxygenclass} FS::SceneManager
:members:
:protected-members:
:undoc-members:
```

(state)=
## State

状態の基底クラス

ヘッダー: `Engine/Framework/StateMachine.h`

```{doxygenclass} State
:members:
:protected-members:
:undoc-members:
```

(statemachine)=
## StateMachine

ステートを管理するマシン

ヘッダー: `Engine/Framework/StateMachine.h`

```{doxygenclass} StateMachine
:members:
:protected-members:
:undoc-members:
```

(fs-terrainchunk)=
## FS::TerrainChunk

ヘッダー: `Engine/Framework/TerrainChunk.h`

```{doxygenclass} FS::TerrainChunk
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### FS::WeatherState

天候の種類

ヘッダー: `Engine/Framework/EnvironmentManager.h`

```{doxygenenum} FS::WeatherState
```

### FS::SceneID

シーンを識別するためのID

ヘッダー: `Engine/Framework/SceneManager.h`

```{doxygenenum} FS::SceneID
```
