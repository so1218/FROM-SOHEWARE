# ゲーム側 (Game)

エンジンを使って作られているゲーム本体のクラスです。エンジンの使い方の実例として参照できます。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FollowCamera <followcamera>` | class | `Game/Camera/FollowCamera.h` |  |
| {ref}`Game <game>` | class | `Game/Game.h` |  |
| {ref}`Ammo <ammo>` | class | `Game/GameObject/Ammo.h` |  |
| {ref}`AmmoManager <ammomanager>` | class | `Game/GameObject/AmmoManager.h` |  |
| {ref}`Enemy <enemy>` | class | `Game/GameObject/Character/Enemy/Enemy.h` |  |
| {ref}`EnemyManager <enemymanager>` | class | `Game/GameObject/Character/Enemy/EnemyManager.h` |  |
| {ref}`FloatingBehavior <floatingbehavior>` | class | `Game/GameObject/Character/Enemy/FloatingBehavior.h` | 浮遊する敵の挙動 |
| {ref}`IEnemyBehavior <ienemybehavior>` | class | `Game/GameObject/Character/Enemy/IEnemyBehavior.h` | 全ての敵の挙動のベースとなるインターフェース |
| {ref}`Player <player>` | class | `Game/GameObject/Character/Player/Player.h` |  |
| {ref}`PlayerReticle <playerreticle>` | class | `Game/GameObject/Character/Player/PlayerReticle.h` |  |
| {ref}`PlayerStateAiming <playerstateaiming>` | class | `Game/GameObject/Character/Player/PlayerStateAiming.h` |  |
| {ref}`PlayerStateJump <playerstatejump>` | class | `Game/GameObject/Character/Player/PlayerStateJump.h` |  |
| {ref}`PlayerStateNormal <playerstatenormal>` | class | `Game/GameObject/Character/Player/PlayerStateNormal.h` |  |
| {ref}`PlayerWeapon <playerweapon>` | class | `Game/GameObject/Character/Player/PlayerWeapon.h` |  |
| {ref}`EnvironmentProp <environmentprop>` | class | `Game/GameObject/EnvironmentProp.h` | ステージ上の配置物（プロップ）を管理するクラス |
| {ref}`PropGroup <propgroup>` | struct | `Game/GameObject/EnvironmentPropManager.h` | プレハブ単位の共有データ |
| {ref}`EnvironmentPropManager <environmentpropmanager>` | class | `Game/GameObject/EnvironmentPropManager.h` | マップ上の環境配置物（プロップ）を一括管理するマネージャークラス |
| {ref}`FoliageField <foliagefield>` | class | `Game/GameObject/FoliageField.h` |  |
| {ref}`GameUI <gameui>` | class | `Game/GameObject/GameUI.h` |  |
| {ref}`GrassField <grassfield>` | class | `Game/GameObject/GrassField.h` |  |
| {ref}`Grid <grid>` | class | `Game/GameObject/Grid.h` |  |
| {ref}`Ground <ground>` | class | `Game/GameObject/Ground.h` |  |
| {ref}`ParticleSystemWrapper <particlesystemwrapper>` | class | `Game/GameObject/ParticleSystemWrapper.h` |  |
| {ref}`PebbleField <pebblefield>` | class | `Game/GameObject/PebbleField.h` |  |
| {ref}`TreeCollider <treecollider>` | struct | `Game/GameObject/TreeField.h` |  |
| {ref}`TreeRaycastHit <treeraycasthit>` | struct | `Game/GameObject/TreeField.h` |  |
| {ref}`TreeField <treefield>` | class | `Game/GameObject/TreeField.h` |  |
| {ref}`WaterManager <watermanager>` | class | `Game/GameObject/WaterManager.h` |  |
| {ref}`WaterObject <waterobject>` | class | `Game/GameObject/WaterObject.h` |  |
| {ref}`WeatherEffectManager <weathereffectmanager>` | class | `Game/GameObject/WeatherEffectManager.h` |  |
| {ref}`WeatherVisualParams <weathervisualparams>` | struct | `Game/GameObject/WeatherEffectManager.h` |  |
| {ref}`PlayScene <playscene>` | class | `Game/Scene/PlayScene.h` |  |
| {ref}`TestScene <testscene>` | class | `Game/Scene/TestScene/TestScene.h` |  |
| {ref}`TitleScene <titlescene>` | class | `Game/Scene/TitleScene.h` |  |

(followcamera)=
## FollowCamera

ヘッダー: `Game/Camera/FollowCamera.h`

```{doxygenclass} FollowCamera
:members:
:protected-members:
:undoc-members:
```

(game)=
## Game

ヘッダー: `Game/Game.h`

```{doxygenclass} Game
:members:
:protected-members:
:undoc-members:
```

(ammo)=
## Ammo

ヘッダー: `Game/GameObject/Ammo.h`

```{doxygenclass} Ammo
:members:
:protected-members:
:undoc-members:
```

(ammomanager)=
## AmmoManager

ヘッダー: `Game/GameObject/AmmoManager.h`

```{doxygenclass} AmmoManager
:members:
:protected-members:
:undoc-members:
```

(enemy)=
## Enemy

ヘッダー: `Game/GameObject/Character/Enemy/Enemy.h`

```{doxygenclass} Enemy
:members:
:protected-members:
:undoc-members:
```

(enemymanager)=
## EnemyManager

ヘッダー: `Game/GameObject/Character/Enemy/EnemyManager.h`

```{doxygenclass} EnemyManager
:members:
:protected-members:
:undoc-members:
```

(floatingbehavior)=
## FloatingBehavior

浮遊する敵の挙動

ヘッダー: `Game/GameObject/Character/Enemy/FloatingBehavior.h`

```{doxygenclass} FloatingBehavior
:members:
:protected-members:
:undoc-members:
```

(ienemybehavior)=
## IEnemyBehavior

全ての敵の挙動のベースとなるインターフェース

ヘッダー: `Game/GameObject/Character/Enemy/IEnemyBehavior.h`

```{doxygenclass} IEnemyBehavior
:members:
:protected-members:
:undoc-members:
```

(player)=
## Player

ヘッダー: `Game/GameObject/Character/Player/Player.h`

```{doxygenclass} Player
:members:
:protected-members:
:undoc-members:
```

(playerreticle)=
## PlayerReticle

ヘッダー: `Game/GameObject/Character/Player/PlayerReticle.h`

```{doxygenclass} PlayerReticle
:members:
:protected-members:
:undoc-members:
```

(playerstateaiming)=
## PlayerStateAiming

ヘッダー: `Game/GameObject/Character/Player/PlayerStateAiming.h`

```{doxygenclass} PlayerStateAiming
:members:
:protected-members:
:undoc-members:
```

(playerstatejump)=
## PlayerStateJump

ヘッダー: `Game/GameObject/Character/Player/PlayerStateJump.h`

```{doxygenclass} PlayerStateJump
:members:
:protected-members:
:undoc-members:
```

(playerstatenormal)=
## PlayerStateNormal

ヘッダー: `Game/GameObject/Character/Player/PlayerStateNormal.h`

```{doxygenclass} PlayerStateNormal
:members:
:protected-members:
:undoc-members:
```

(playerweapon)=
## PlayerWeapon

ヘッダー: `Game/GameObject/Character/Player/PlayerWeapon.h`

```{doxygenclass} PlayerWeapon
:members:
:protected-members:
:undoc-members:
```

(environmentprop)=
## EnvironmentProp

ステージ上の配置物（プロップ）を管理するクラス

ヘッダー: `Game/GameObject/EnvironmentProp.h`

```{doxygenclass} EnvironmentProp
:members:
:protected-members:
:undoc-members:
```

(propgroup)=
## PropGroup

プレハブ単位の共有データ

ヘッダー: `Game/GameObject/EnvironmentPropManager.h`

```{doxygenstruct} PropGroup
:members:
:protected-members:
:undoc-members:
```

(environmentpropmanager)=
## EnvironmentPropManager

マップ上の環境配置物（プロップ）を一括管理するマネージャークラス

ヘッダー: `Game/GameObject/EnvironmentPropManager.h`

```{doxygenclass} EnvironmentPropManager
:members:
:protected-members:
:undoc-members:
```

(foliagefield)=
## FoliageField

ヘッダー: `Game/GameObject/FoliageField.h`

```{doxygenclass} FoliageField
:members:
:protected-members:
:undoc-members:
```

(gameui)=
## GameUI

ヘッダー: `Game/GameObject/GameUI.h`

```{doxygenclass} GameUI
:members:
:protected-members:
:undoc-members:
```

(grassfield)=
## GrassField

ヘッダー: `Game/GameObject/GrassField.h`

```{doxygenclass} GrassField
:members:
:protected-members:
:undoc-members:
```

(grid)=
## Grid

ヘッダー: `Game/GameObject/Grid.h`

```{doxygenclass} Grid
:members:
:protected-members:
:undoc-members:
```

(ground)=
## Ground

ヘッダー: `Game/GameObject/Ground.h`

```{doxygenclass} Ground
:members:
:protected-members:
:undoc-members:
```

(particlesystemwrapper)=
## ParticleSystemWrapper

ヘッダー: `Game/GameObject/ParticleSystemWrapper.h`

```{doxygenclass} ParticleSystemWrapper
:members:
:protected-members:
:undoc-members:
```

(pebblefield)=
## PebbleField

ヘッダー: `Game/GameObject/PebbleField.h`

```{doxygenclass} PebbleField
:members:
:protected-members:
:undoc-members:
```

(treecollider)=
## TreeCollider

ヘッダー: `Game/GameObject/TreeField.h`

```{doxygenstruct} TreeCollider
:members:
:protected-members:
:undoc-members:
```

(treeraycasthit)=
## TreeRaycastHit

ヘッダー: `Game/GameObject/TreeField.h`

```{doxygenstruct} TreeRaycastHit
:members:
:protected-members:
:undoc-members:
```

(treefield)=
## TreeField

ヘッダー: `Game/GameObject/TreeField.h`

```{doxygenclass} TreeField
:members:
:protected-members:
:undoc-members:
```

(watermanager)=
## WaterManager

ヘッダー: `Game/GameObject/WaterManager.h`

```{doxygenclass} WaterManager
:members:
:protected-members:
:undoc-members:
```

(waterobject)=
## WaterObject

ヘッダー: `Game/GameObject/WaterObject.h`

```{doxygenclass} WaterObject
:members:
:protected-members:
:undoc-members:
```

(weathereffectmanager)=
## WeatherEffectManager

ヘッダー: `Game/GameObject/WeatherEffectManager.h`

```{doxygenclass} WeatherEffectManager
:members:
:protected-members:
:undoc-members:
```

(weathervisualparams)=
## WeatherVisualParams

ヘッダー: `Game/GameObject/WeatherEffectManager.h`

```{doxygenstruct} WeatherVisualParams
:members:
:protected-members:
:undoc-members:
```

(playscene)=
## PlayScene

ヘッダー: `Game/Scene/PlayScene.h`

```{doxygenclass} PlayScene
:members:
:protected-members:
:undoc-members:
```

(testscene)=
## TestScene

ヘッダー: `Game/Scene/TestScene/TestScene.h`

```{doxygenclass} TestScene
:members:
:protected-members:
:undoc-members:
```

(titlescene)=
## TitleScene

ヘッダー: `Game/Scene/TitleScene.h`

```{doxygenclass} TitleScene
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### ObjectUpdateOrder::Priority

ヘッダー: `Game/Global/GameDefine.h`

```{doxygenenum} ObjectUpdateOrder::Priority
```

### ObjectTag::ID

ヘッダー: `Game/Global/GameDefine.h`

```{doxygenenum} ObjectTag::ID
```

## 衝突属性の定数

ヘッダー: `Game/Global/CollisionConfig.h`

```{doxygenfile} CollisionConfig.h
```
