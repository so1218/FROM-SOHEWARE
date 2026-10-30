# グラフィックス: 描画システム (Graphics/RenderSystem)

`Model` `Sprite` `Line` など描画したいものを表すクラスと、PSO・ルートシグネチャ管理、フレーム全体の描画手順 (`RenderPipeline`) です。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::FoliageSystem <fs-foliagesystem>` | class | `Engine/Graphics/RenderSystem/FoliageSystem.h` |  |
| {ref}`FS::GrassSystem <fs-grasssystem>` | class | `Engine/Graphics/RenderSystem/GrassSystem.h` |  |
| {ref}`FS::LightManager <fs-lightmanager>` | class | `Engine/Graphics/RenderSystem/LightManager.h` |  |
| {ref}`FS::LightningSystem <fs-lightningsystem>` | class | `Engine/Graphics/RenderSystem/LightningSystem.h` |  |
| {ref}`FS::Line <fs-line>` | class | `Engine/Graphics/RenderSystem/Line.h` |  |
| {ref}`FS::Model <fs-model>` | class | `Engine/Graphics/RenderSystem/Model.h` |  |
| {ref}`FS::PSODescription <fs-psodescription>` | struct | `Engine/Graphics/RenderSystem/PSOManager.h` | JSONで指定するPSO設定 |
| {ref}`FS::PSOManager <fs-psomanager>` | class | `Engine/Graphics/RenderSystem/PSOManager.h` |  |
| {ref}`FS::PebbleSystem <fs-pebblesystem>` | class | `Engine/Graphics/RenderSystem/PebbleSystem.h` |  |
| {ref}`FS::RenderData <fs-renderdata>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::ModelSubmission <fs-modelsubmission>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` | 描画要求情報（レンダリング用の1オブジェクト単位データ） |
| {ref}`FS::RenderBatch <fs-renderbatch>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::SpriteSubmission <fs-spritesubmission>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::WaterSubmission <fs-watersubmission>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::WaterBatch <fs-waterbatch>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::RenderSettings <fs-rendersettings>` | struct | `Engine/Graphics/RenderSystem/RenderCommon.h` |  |
| {ref}`FS::RenderCoordinator <fs-rendercoordinator>` | class | `Engine/Graphics/RenderSystem/RenderCoordinator.h` |  |
| {ref}`FS::RenderCameraState <fs-rendercamerastate>` | struct | `Engine/Graphics/RenderSystem/RenderPipeline.h` | カメラ情報をまとめる構造体 |
| {ref}`FS::RenderPipeline <fs-renderpipeline>` | class | `Engine/Graphics/RenderSystem/RenderPipeline.h` |  |
| {ref}`FS::RootSignatureBuilder <fs-rootsignaturebuilder>` | class | `Engine/Graphics/RenderSystem/RootSignatureBuilder.h` |  |
| {ref}`FS::RootSignatureManager <fs-rootsignaturemanager>` | class | `Engine/Graphics/RenderSystem/RootSignatureManager.h` |  |
| {ref}`FS::Skybox <fs-skybox>` | class | `Engine/Graphics/RenderSystem/Skybox.h` |  |
| {ref}`FS::Skydome <fs-skydome>` | class | `Engine/Graphics/RenderSystem/Skydome.h` |  |
| {ref}`FS::Sprite <fs-sprite>` | class | `Engine/Graphics/RenderSystem/Sprite.h` |  |
| {ref}`FS::Terrain <fs-terrain>` | class | `Engine/Graphics/RenderSystem/Terrain.h` |  |
| {ref}`FS::TreeInstance <fs-treeinstance>` | struct | `Engine/Graphics/RenderSystem/TreeSystem.h` | 木のインスタンス情報 |
| {ref}`FS::TreeSystem <fs-treesystem>` | class | `Engine/Graphics/RenderSystem/TreeSystem.h` |  |
| {ref}`FS::WorldInteractionSystem <fs-worldinteractionsystem>` | class | `Engine/Graphics/RenderSystem/WorldInteractionSystem.h` |  |

(fs-foliagesystem)=
## FS::FoliageSystem

ヘッダー: `Engine/Graphics/RenderSystem/FoliageSystem.h`

```{doxygenclass} FS::FoliageSystem
:members:
:protected-members:
:undoc-members:
```

(fs-grasssystem)=
## FS::GrassSystem

ヘッダー: `Engine/Graphics/RenderSystem/GrassSystem.h`

```{doxygenclass} FS::GrassSystem
:members:
:protected-members:
:undoc-members:
```

(fs-lightmanager)=
## FS::LightManager

ヘッダー: `Engine/Graphics/RenderSystem/LightManager.h`

```{doxygenclass} FS::LightManager
:members:
:protected-members:
:undoc-members:
```

(fs-lightningsystem)=
## FS::LightningSystem

ヘッダー: `Engine/Graphics/RenderSystem/LightningSystem.h`

```{doxygenclass} FS::LightningSystem
:members:
:protected-members:
:undoc-members:
```

(fs-line)=
## FS::Line

ヘッダー: `Engine/Graphics/RenderSystem/Line.h`

```{doxygenclass} FS::Line
:members:
:protected-members:
:undoc-members:
```

(fs-model)=
## FS::Model

ヘッダー: `Engine/Graphics/RenderSystem/Model.h`

```{doxygenclass} FS::Model
:members:
:protected-members:
:undoc-members:
```

(fs-psodescription)=
## FS::PSODescription

JSONで指定するPSO設定

ヘッダー: `Engine/Graphics/RenderSystem/PSOManager.h`

```{doxygenstruct} FS::PSODescription
:members:
:protected-members:
:undoc-members:
```

(fs-psomanager)=
## FS::PSOManager

ヘッダー: `Engine/Graphics/RenderSystem/PSOManager.h`

```{doxygenclass} FS::PSOManager
:members:
:protected-members:
:undoc-members:
```

(fs-pebblesystem)=
## FS::PebbleSystem

ヘッダー: `Engine/Graphics/RenderSystem/PebbleSystem.h`

```{doxygenclass} FS::PebbleSystem
:members:
:protected-members:
:undoc-members:
```

(fs-renderdata)=
## FS::RenderData

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::RenderData
:members:
:protected-members:
:undoc-members:
```

(fs-modelsubmission)=
## FS::ModelSubmission

描画要求情報（レンダリング用の1オブジェクト単位データ）

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::ModelSubmission
:members:
:protected-members:
:undoc-members:
```

(fs-renderbatch)=
## FS::RenderBatch

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::RenderBatch
:members:
:protected-members:
:undoc-members:
```

(fs-spritesubmission)=
## FS::SpriteSubmission

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::SpriteSubmission
:members:
:protected-members:
:undoc-members:
```

(fs-watersubmission)=
## FS::WaterSubmission

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::WaterSubmission
:members:
:protected-members:
:undoc-members:
```

(fs-waterbatch)=
## FS::WaterBatch

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::WaterBatch
:members:
:protected-members:
:undoc-members:
```

(fs-rendersettings)=
## FS::RenderSettings

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenstruct} FS::RenderSettings
:members:
:protected-members:
:undoc-members:
```

(fs-rendercoordinator)=
## FS::RenderCoordinator

ヘッダー: `Engine/Graphics/RenderSystem/RenderCoordinator.h`

```{doxygenclass} FS::RenderCoordinator
:members:
:protected-members:
:undoc-members:
```

(fs-rendercamerastate)=
## FS::RenderCameraState

カメラ情報をまとめる構造体

ヘッダー: `Engine/Graphics/RenderSystem/RenderPipeline.h`

```{doxygenstruct} FS::RenderCameraState
:members:
:protected-members:
:undoc-members:
```

(fs-renderpipeline)=
## FS::RenderPipeline

ヘッダー: `Engine/Graphics/RenderSystem/RenderPipeline.h`

```{doxygenclass} FS::RenderPipeline
:members:
:protected-members:
:undoc-members:
```

(fs-rootsignaturebuilder)=
## FS::RootSignatureBuilder

ヘッダー: `Engine/Graphics/RenderSystem/RootSignatureBuilder.h`

```{doxygenclass} FS::RootSignatureBuilder
:members:
:protected-members:
:undoc-members:
```

(fs-rootsignaturemanager)=
## FS::RootSignatureManager

ヘッダー: `Engine/Graphics/RenderSystem/RootSignatureManager.h`

```{doxygenclass} FS::RootSignatureManager
:members:
:protected-members:
:undoc-members:
```

(fs-skybox)=
## FS::Skybox

ヘッダー: `Engine/Graphics/RenderSystem/Skybox.h`

```{doxygenclass} FS::Skybox
:members:
:protected-members:
:undoc-members:
```

(fs-skydome)=
## FS::Skydome

ヘッダー: `Engine/Graphics/RenderSystem/Skydome.h`

```{doxygenclass} FS::Skydome
:members:
:protected-members:
:undoc-members:
```

(fs-sprite)=
## FS::Sprite

ヘッダー: `Engine/Graphics/RenderSystem/Sprite.h`

```{doxygenclass} FS::Sprite
:members:
:protected-members:
:undoc-members:
```

(fs-terrain)=
## FS::Terrain

ヘッダー: `Engine/Graphics/RenderSystem/Terrain.h`

```{doxygenclass} FS::Terrain
:members:
:protected-members:
:undoc-members:
```

(fs-treeinstance)=
## FS::TreeInstance

木のインスタンス情報

ヘッダー: `Engine/Graphics/RenderSystem/TreeSystem.h`

```{doxygenstruct} FS::TreeInstance
:members:
:protected-members:
:undoc-members:
```

(fs-treesystem)=
## FS::TreeSystem

ヘッダー: `Engine/Graphics/RenderSystem/TreeSystem.h`

```{doxygenclass} FS::TreeSystem
:members:
:protected-members:
:undoc-members:
```

(fs-worldinteractionsystem)=
## FS::WorldInteractionSystem

ヘッダー: `Engine/Graphics/RenderSystem/WorldInteractionSystem.h`

```{doxygenclass} FS::WorldInteractionSystem
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### FS::SelectedLightType

ライトの種類を表す列挙型

ヘッダー: `Engine/Graphics/RenderSystem/LightManager.h`

```{doxygenenum} FS::SelectedLightType
```

### FS::RenderType

描画するオブジェクトの種類

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenenum} FS::RenderType
```

### FS::RenderGroup

描画グループ（描画順の優先度や用途で分類）

ヘッダー: `Engine/Graphics/RenderSystem/RenderCommon.h`

```{doxygenenum} FS::RenderGroup
```
