# グラフィックス: リソース (Graphics/Resource)

SRV/RTV/DSV の割り当て、テクスチャ読み込み、シェーダーコンパイル、シャドウマップ、定数バッファなど GPU リソースの管理です。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::DSVManager <fs-dsvmanager>` | class | `Engine/Graphics/Resource/DSVManager.h` |  |
| {ref}`FS::GlobalConstants <fs-globalconstants>` | class | `Engine/Graphics/Resource/GlobalConstants.h` | シーン全体で共有する定数バッファの管理 |
| {ref}`FS::MaterialManager <fs-materialmanager>` | class | `Engine/Graphics/Resource/MaterialManager.h` |  |
| {ref}`FS::RTVManager <fs-rtvmanager>` | class | `Engine/Graphics/Resource/RTVManager.h` |  |
| {ref}`FS::OffscreenRTVManager <fs-offscreenrtvmanager>` | class | `Engine/Graphics/Resource/RTVManager.h` |  |
| {ref}`FS::SRVAllocator <fs-srvallocator>` | class | `Engine/Graphics/Resource/SRVAllocator.h` |  |
| {ref}`FS::SRVManager <fs-srvmanager>` | class | `Engine/Graphics/Resource/SRVManager.h` |  |
| {ref}`FS::ShaderManager <fs-shadermanager>` | class | `Engine/Graphics/Resource/ShaderManager.h` |  |
| {ref}`FS::ShadowMap <fs-shadowmap>` | class | `Engine/Graphics/Resource/ShadowMap.h` |  |
| {ref}`FS::VertexData <fs-vertexdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::TrailVertexData <fs-trailvertexdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::LineVertexData <fs-linevertexdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::TerrainVertexData <fs-terrainvertexdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::LightningVertexData <fs-lightningvertexdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::TextureData <fs-texturedata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::AABB <fs-aabb>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::MaterialHandle <fs-materialhandle>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::TreeMaterialHandle <fs-treematerialhandle>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::VertexWeightData <fs-vertexweightdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::JointWeightData <fs-jointweightdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::Node <fs-node>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::MeshData <fs-meshdata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::ModelData <fs-modeldata>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::LightningConfig <fs-lightningconfig>` | struct | `Engine/Graphics/Resource/Structures.h` | シェーダーに送る雷の見た目パラメータ |
| {ref}`FS::FoliageLayer <fs-foliagelayer>` | struct | `Engine/Graphics/Resource/Structures.h` |  |
| {ref}`FS::TextureLoader <fs-textureloader>` | class | `Engine/Graphics/Resource/TextureLoader.h` |  |

(fs-dsvmanager)=
## FS::DSVManager

ヘッダー: `Engine/Graphics/Resource/DSVManager.h`

```{doxygenclass} FS::DSVManager
:members:
:protected-members:
:undoc-members:
```

(fs-globalconstants)=
## FS::GlobalConstants

シーン全体で共有する定数バッファの管理

ヘッダー: `Engine/Graphics/Resource/GlobalConstants.h`

```{doxygenclass} FS::GlobalConstants
:members:
:protected-members:
:undoc-members:
```

(fs-materialmanager)=
## FS::MaterialManager

ヘッダー: `Engine/Graphics/Resource/MaterialManager.h`

```{doxygenclass} FS::MaterialManager
:members:
:protected-members:
:undoc-members:
```

(fs-rtvmanager)=
## FS::RTVManager

ヘッダー: `Engine/Graphics/Resource/RTVManager.h`

```{doxygenclass} FS::RTVManager
:members:
:protected-members:
:undoc-members:
```

(fs-offscreenrtvmanager)=
## FS::OffscreenRTVManager

ヘッダー: `Engine/Graphics/Resource/RTVManager.h`

```{doxygenclass} FS::OffscreenRTVManager
:members:
:protected-members:
:undoc-members:
```

(fs-srvallocator)=
## FS::SRVAllocator

ヘッダー: `Engine/Graphics/Resource/SRVAllocator.h`

```{doxygenclass} FS::SRVAllocator
:members:
:protected-members:
:undoc-members:
```

(fs-srvmanager)=
## FS::SRVManager

ヘッダー: `Engine/Graphics/Resource/SRVManager.h`

```{doxygenclass} FS::SRVManager
:members:
:protected-members:
:undoc-members:
```

(fs-shadermanager)=
## FS::ShaderManager

ヘッダー: `Engine/Graphics/Resource/ShaderManager.h`

```{doxygenclass} FS::ShaderManager
:members:
:protected-members:
:undoc-members:
```

(fs-shadowmap)=
## FS::ShadowMap

ヘッダー: `Engine/Graphics/Resource/ShadowMap.h`

```{doxygenclass} FS::ShadowMap
:members:
:protected-members:
:undoc-members:
```

(fs-vertexdata)=
## FS::VertexData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::VertexData
:members:
:protected-members:
:undoc-members:
```

(fs-trailvertexdata)=
## FS::TrailVertexData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::TrailVertexData
:members:
:protected-members:
:undoc-members:
```

(fs-linevertexdata)=
## FS::LineVertexData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::LineVertexData
:members:
:protected-members:
:undoc-members:
```

(fs-terrainvertexdata)=
## FS::TerrainVertexData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::TerrainVertexData
:members:
:protected-members:
:undoc-members:
```

(fs-lightningvertexdata)=
## FS::LightningVertexData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::LightningVertexData
:members:
:protected-members:
:undoc-members:
```

(fs-texturedata)=
## FS::TextureData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::TextureData
:members:
:protected-members:
:undoc-members:
```

(fs-aabb)=
## FS::AABB

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::AABB
:members:
:protected-members:
:undoc-members:
```

(fs-materialhandle)=
## FS::MaterialHandle

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::MaterialHandle
:members:
:protected-members:
:undoc-members:
```

(fs-treematerialhandle)=
## FS::TreeMaterialHandle

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::TreeMaterialHandle
:members:
:protected-members:
:undoc-members:
```

(fs-vertexweightdata)=
## FS::VertexWeightData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::VertexWeightData
:members:
:protected-members:
:undoc-members:
```

(fs-jointweightdata)=
## FS::JointWeightData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::JointWeightData
:members:
:protected-members:
:undoc-members:
```

(fs-node)=
## FS::Node

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::Node
:members:
:protected-members:
:undoc-members:
```

(fs-meshdata)=
## FS::MeshData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::MeshData
:members:
:protected-members:
:undoc-members:
```

(fs-modeldata)=
## FS::ModelData

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::ModelData
:members:
:protected-members:
:undoc-members:
```

(fs-lightningconfig)=
## FS::LightningConfig

シェーダーに送る雷の見た目パラメータ

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::LightningConfig
:members:
:protected-members:
:undoc-members:
```

(fs-foliagelayer)=
## FS::FoliageLayer

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenstruct} FS::FoliageLayer
:members:
:protected-members:
:undoc-members:
```

(fs-textureloader)=
## FS::TextureLoader

ヘッダー: `Engine/Graphics/Resource/TextureLoader.h`

```{doxygenclass} FS::TextureLoader
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### FS::GBufferIndex

ヘッダー: `Engine/Graphics/Resource/RTVManager.h`

```{doxygenenum} FS::GBufferIndex
```

### FS::CullMode

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenenum} FS::CullMode
```

### FS::DepthMode

ヘッダー: `Engine/Graphics/Resource/Structures.h`

```{doxygenenum} FS::DepthMode
```

### FS::TextureType

ヘッダー: `Engine/Graphics/Resource/TextureLoader.h`

```{doxygenenum} FS::TextureType
```
