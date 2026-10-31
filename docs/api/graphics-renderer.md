# グラフィックス: レンダラー (Graphics/Renderer)

描画要求を種類ごとにまとめて実際に GPU へ描画コマンドを積むレンダラー群と、それらを束ねる `RendererManager` です。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::FoliageTypeConfig <fs-foliagetypeconfig>` | struct | `Engine/Graphics/Renderer/FoliageRenderer.h` |  |
| {ref}`FS::FoliageRenderer <fs-foliagerenderer>` | class | `Engine/Graphics/Renderer/FoliageRenderer.h` |  |
| {ref}`FS::GrassRenderer <fs-grassrenderer>` | class | `Engine/Graphics/Renderer/GrassRenderer.h` |  |
| {ref}`FS::LightningPath <fs-lightningpath>` | struct | `Engine/Graphics/Renderer/LightningRenderer.h` | 新しく1本の線を定義する構造体 |
| {ref}`FS::LightningInstance <fs-lightninginstance>` | struct | `Engine/Graphics/Renderer/LightningRenderer.h` | 1本の雷のデータを管理する構造体 |
| {ref}`FS::LightningRenderer <fs-lightningrenderer>` | class | `Engine/Graphics/Renderer/LightningRenderer.h` |  |
| {ref}`FS::LineRenderer <fs-linerenderer>` | class | `Engine/Graphics/Renderer/LineRenderer.h` |  |
| {ref}`FS::ModelRenderer <fs-modelrenderer>` | class | `Engine/Graphics/Renderer/ModelRenderer.h` |  |
| {ref}`FS::ParticleRequest <fs-particlerequest>` | struct | `Engine/Graphics/Renderer/ParticleRenderer.h` | 1つのパーティクルの描画に必要な情報をまとめたもの |
| {ref}`FS::ParticleRenderer <fs-particlerenderer>` | class | `Engine/Graphics/Renderer/ParticleRenderer.h` |  |
| {ref}`FS::RenderEnvironment <fs-renderenvironment>` | struct | `Engine/Graphics/Renderer/RenderEnvironment.h` |  |
| {ref}`FS::WorldInteractionData <fs-worldinteractiondata>` | struct | `Engine/Graphics/Renderer/RendererManager.h` | インタラクションデータ保持用の構造体 |
| {ref}`FS::RendererManager <fs-renderermanager>` | class | `Engine/Graphics/Renderer/RendererManager.h` |  |
| {ref}`FS::SkyboxRenderer <fs-skyboxrenderer>` | class | `Engine/Graphics/Renderer/SkyboxRenderer.h` |  |
| {ref}`FS::SkydomeRenderer <fs-skydomerenderer>` | class | `Engine/Graphics/Renderer/SkydomeRenderer.h` |  |
| {ref}`FS::SpriteRenderer <fs-spriterenderer>` | class | `Engine/Graphics/Renderer/SpriteRenderer.h` |  |
| {ref}`FS::TerrainRenderer <fs-terrainrenderer>` | class | `Engine/Graphics/Renderer/TerrainRenderer.h` |  |
| {ref}`FS::TrailRenderer <fs-trailrenderer>` | class | `Engine/Graphics/Renderer/TrailRenderer.h` |  |
| {ref}`FS::TreeRenderer <fs-treerenderer>` | class | `Engine/Graphics/Renderer/TreeRenderer.h` |  |
| {ref}`FS::WaterRenderer <fs-waterrenderer>` | class | `Engine/Graphics/Renderer/WaterRenderer.h` |  |

(fs-foliagetypeconfig)=
## FS::FoliageTypeConfig

ヘッダー: `Engine/Graphics/Renderer/FoliageRenderer.h`

```{doxygenstruct} FS::FoliageTypeConfig
:members:
:protected-members:
:undoc-members:
```

(fs-foliagerenderer)=
## FS::FoliageRenderer

ヘッダー: `Engine/Graphics/Renderer/FoliageRenderer.h`

```{doxygenclass} FS::FoliageRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-grassrenderer)=
## FS::GrassRenderer

ヘッダー: `Engine/Graphics/Renderer/GrassRenderer.h`

```{doxygenclass} FS::GrassRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-lightningpath)=
## FS::LightningPath

新しく1本の線を定義する構造体

ヘッダー: `Engine/Graphics/Renderer/LightningRenderer.h`

```{doxygenstruct} FS::LightningPath
:members:
:protected-members:
:undoc-members:
```

(fs-lightninginstance)=
## FS::LightningInstance

1本の雷のデータを管理する構造体

ヘッダー: `Engine/Graphics/Renderer/LightningRenderer.h`

```{doxygenstruct} FS::LightningInstance
:members:
:protected-members:
:undoc-members:
```

(fs-lightningrenderer)=
## FS::LightningRenderer

ヘッダー: `Engine/Graphics/Renderer/LightningRenderer.h`

```{doxygenclass} FS::LightningRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-linerenderer)=
## FS::LineRenderer

ヘッダー: `Engine/Graphics/Renderer/LineRenderer.h`

```{doxygenclass} FS::LineRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-modelrenderer)=
## FS::ModelRenderer

ヘッダー: `Engine/Graphics/Renderer/ModelRenderer.h`

```{doxygenclass} FS::ModelRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-particlerequest)=
## FS::ParticleRequest

1つのパーティクルの描画に必要な情報をまとめたもの

ヘッダー: `Engine/Graphics/Renderer/ParticleRenderer.h`

```{doxygenstruct} FS::ParticleRequest
:members:
:protected-members:
:undoc-members:
```

(fs-particlerenderer)=
## FS::ParticleRenderer

ヘッダー: `Engine/Graphics/Renderer/ParticleRenderer.h`

```{doxygenclass} FS::ParticleRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-renderenvironment)=
## FS::RenderEnvironment

ヘッダー: `Engine/Graphics/Renderer/RenderEnvironment.h`

```{doxygenstruct} FS::RenderEnvironment
:members:
:protected-members:
:undoc-members:
```

(fs-worldinteractiondata)=
## FS::WorldInteractionData

インタラクションデータ保持用の構造体

ヘッダー: `Engine/Graphics/Renderer/RendererManager.h`

```{doxygenstruct} FS::WorldInteractionData
:members:
:protected-members:
:undoc-members:
```

(fs-renderermanager)=
## FS::RendererManager

ヘッダー: `Engine/Graphics/Renderer/RendererManager.h`

```{doxygenclass} FS::RendererManager
:members:
:protected-members:
:undoc-members:
```

(fs-skyboxrenderer)=
## FS::SkyboxRenderer

ヘッダー: `Engine/Graphics/Renderer/SkyboxRenderer.h`

```{doxygenclass} FS::SkyboxRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-skydomerenderer)=
## FS::SkydomeRenderer

ヘッダー: `Engine/Graphics/Renderer/SkydomeRenderer.h`

```{doxygenclass} FS::SkydomeRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-spriterenderer)=
## FS::SpriteRenderer

ヘッダー: `Engine/Graphics/Renderer/SpriteRenderer.h`

```{doxygenclass} FS::SpriteRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-terrainrenderer)=
## FS::TerrainRenderer

ヘッダー: `Engine/Graphics/Renderer/TerrainRenderer.h`

```{doxygenclass} FS::TerrainRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-trailrenderer)=
## FS::TrailRenderer

ヘッダー: `Engine/Graphics/Renderer/TrailRenderer.h`

```{doxygenclass} FS::TrailRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-treerenderer)=
## FS::TreeRenderer

ヘッダー: `Engine/Graphics/Renderer/TreeRenderer.h`

```{doxygenclass} FS::TreeRenderer
:members:
:protected-members:
:undoc-members:
```

(fs-waterrenderer)=
## FS::WaterRenderer

ヘッダー: `Engine/Graphics/Renderer/WaterRenderer.h`

```{doxygenclass} FS::WaterRenderer
:members:
:protected-members:
:undoc-members:
```
