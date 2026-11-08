# アセットとデータファイル

本ドキュメントでは、モデル・テクスチャ・アニメーション・音声をエンジンに登録する方法と、`Assets/Data/` に置かれている設定ファイルの役割を解説します。
エンジンのアセットは **名前** で管理されており、ゲームのコードからはファイルパスではなく登録した名前で指定します。

:::{note}
本ドキュメントのパスは、すべて実行時の作業ディレクトリである `project/` からの相対パスです。
:::

---

## 目次
- [1. フォルダ構成](#1-フォルダ構成)
- [2. モデルとアニメーション (ModelList.csv / AnimationList.csv)](#2-モデルとアニメーション-modellistcsv--animationlistcsv)
- [3. 音声 (AudioList.csv)](#3-音声-audiolistcsv)
- [4. テクスチャ (自動読み込み)](#4-テクスチャ-自動読み込み)
- [5. シェーダーと PSO 定義](#5-シェーダーと-pso-定義)
- [6. キャッシュフォルダ](#6-キャッシュフォルダ)

---

## 1. フォルダ構成

```text
project/
├── Assets/
│   ├── Audio/              音声ファイル (AudioList.csv から参照)
│   ├── Models/             モデル・アニメーション (ModelList.csv / AnimationList.csv から参照)
│   ├── Textures/           テクスチャ (フォルダごと自動で読み込まれる)
│   ├── Shaders/            HLSL シェーダー
│   └── Data/
│       ├── ModelList.csv       モデルの登録表
│       ├── AnimationList.csv   アニメーションの登録表
│       ├── AudioList.csv       音声の登録表
│       ├── PSODefinitions/     パイプラインステート (PSO) の定義 (JSON)
│       ├── GlobalVariables/    PropertyBinder で保存したパラメータ (JSON)
│       ├── Particle/           パーティクルのプリセット (JSON)
│       └── Fonts/              ImGui 用フォント
└── Cache/
    ├── Shaders/            コンパイル済みシェーダー (.cso)
    └── Textures/           変換済みテクスチャ (.dds)
```

---

## 2. モデルとアニメーション (ModelList.csv / AnimationList.csv)

モデルは `Assets/Data/ModelList.csv`、アニメーションは `Assets/Data/AnimationList.csv` に 1 行ずつ登録します。どちらも起動時 (`Engine::Initialize()`) に一括で読み込まれます。

```text
Name,Path
# Primitives
cube,Primitives/Cube/normalCube.obj
sphere,Primitives/Sphere/sphere.obj
plane,Primitives/Plane/plane.obj
```

- 1 行目はヘッダー行として読み飛ばされます。
- 2 列目のパスは `Assets/Models/` からの相対パスです。
- `#` または `//` で始まる行と空行は無視されるので、分類用の見出しに使えます。
- 読み込みには Assimp を使用しています (`.obj`、`.gltf` など)。

登録した名前は、`Model` や `AnimationModel` の生成時に指定します。

```cpp
model_ = std::make_unique<FS::Model>(engine_, "sphere");
animationModel_ = std::make_unique<FS::AnimationModel>(engine_, "humanMesh", "humanRun");
```

---

## 3. 音声 (AudioList.csv)

音声は `Assets/Data/AudioList.csv` に登録します。書式はモデルと同じで、パスは `Assets/Audio/` からの相対パスです。デコードには Media Foundation を使っているため、`.mp3` なども読み込めます。

```text
Name,Path
# BGM
sunnyBGM,BGM/sunnyBGM.mp3
rainBGM,BGM/rainBGM.mp3
```

```cpp
FS::AudioPlayer::GetInstance().PlayUnique("sunnyBGM", true, 50);
```

再生方法は [入力・時間・音声・ログ](systemsGuide.md#3-音声再生-audioplayer) を参照してください。

---

## 4. テクスチャ (自動読み込み)

テクスチャは登録表を使わず、起動時に `Assets/Textures/` 以下のフォルダがすべて走査され、見つかった画像 (`.png` `.jpg` `.tga` `.dds`) が自動で読み込まれます。

- **名前は拡張子を除いたファイル名** です（例: `Assets/Textures/UI/titleLogo.png` → `"titleLogo"`）。大文字・小文字は区別されません。
- 名前が重複したファイルは、2 つ目以降が読み込まれません（コンソールに警告が出ます）。フォルダが違っても重複扱いになるので注意してください。
- 指定した名前が見つからない場合は `white1x1` が代わりに使われます。

テクスチャの種類 (`TextureType`) は、置いたフォルダ名やファイル名から自動で判定されます。`PropertyBinder::BindTexture()` の候補の絞り込みなどに使われます。

| 判定条件 (パスに含まれる文字列) | 種類 |
| :--- | :--- |
| `/albedo/`、`/utility/` | `Albedo` |
| `/particles/` | `Particle` |
| `/ui/` | `UI` |
| `/environments/`、`/cubemap/` | `CubeMap` |
| `/normal/`、またはファイル名が `_n` / `_normal` で終わる | `Normal` |
| `/ramps/` | `Toon` |
| `/noise/` | `Noise` |
| `/luts/` | `LUT` |
| 上記以外 | `Albedo` |

`.dds` 以外の画像は初回の読み込み時に `.dds` に変換され、`Cache/Textures/` に保存されます。2 回目以降の起動ではこのキャッシュが使われるため、読み込みが速くなります。

---

## 5. シェーダーと PSO 定義

### 5-1. PSO 定義ファイル

パイプラインステート (PSO) は、`Assets/Data/PSODefinitions/<PSO名>.json` に JSON で定義します。PSO は最初に使われたときに、この JSON を読んで生成されます。

```json
{
  "RootSignature": "Instancing3D",
  "VertexShader": "Objects/Object3D_Instanced.VS.hlsl",
  "PixelShader": "Objects/Object3D.PS.hlsl",
  "InputLayout": "Default3D",
  "BlendState": "Opaque",
  "RasterizerState": "BackCullSolid",
  "DepthStencilState": "Default",
  "Topology": "Triangle",
  "RTVFormats": [
    "R16G16B16A16_FLOAT",
    "R16G16B16A16_FLOAT",
    "R8G8B8A8_UNORM",
    "R16G16_FLOAT"
  ],
  "DSVFormat": "D24_UNORM_S8_UINT"
}
```

| キー | 必須 | 指定できる値 |
| :--- | :---: | :--- |
| `RootSignature` | ✓ | `RootSignatureManager` に登録されている名前 (`Instancing3D`、`Skinning`、`PostProcess`、`Sprite` など) |
| `VertexShader` | グラフィックス PSO では ✓ | `Assets/Shaders/` からの相対パス |
| `PixelShader` | | 同上。深度のみのパスでは省略可 |
| `ComputeShader` | | 指定するとコンピュートパイプラインになる |
| `InputLayout` | | `Default3D` `Skinning` `Depth` `Skybox` `Trail` `Line` `Grass` `Tree` `Skydome` `Terrain` `Lightning` `Fullscreen` `None` |
| `BlendState` | | `Opaque` `OpaqueA2C` `AlphaBlend` `AlphaBlendUI` `Add` `Additive` `Subtract` `Multiply` `Screen` `Exclusion` `FinalComposite` |
| `RasterizerState` | | `BackCullSolid` `FrontCullSolid` `NoCullSolid` `FrontCullBias` `Wireframe` `LineAA` |
| `DepthStencilState` | | `Default` `ReadOnly` `DepthOnly` `Off` |
| `Topology` | | `Triangle` (既定) / `Line` |
| `RTVFormats` | | `R16G16B16A16_FLOAT` `R16G16_FLOAT` `R8G8B8A8_UNORM` `R8G8B8A8_UNORM_SRGB` `R8_UNORM` `UNKNOWN` |
| `DSVFormat` | | `D24_UNORM_S8_UINT` (既定) / `D32_FLOAT` / `UNKNOWN` |

:::{note}
ルートシグネチャは JSON ではなく C++ (`Engine/Graphics/RenderSystem/RootSignatureManager.cpp`) で定義されています。既存のルートシグネチャで足りない場合は、そちらに追加してから JSON で名前を指定してください。
:::

### 5-2. シェーダーのコンパイルとキャッシュ

シェーダーは DXC でコンパイルされます。エントリーポイントは `main`、プロファイルは `vs_6_0` / `ps_6_0` / `cs_6_0` です。`#include` の基準フォルダは `Assets/Shaders/` です。

| ビルド | 動作 |
| :--- | :--- |
| `ENABLE_DEV_TOOLS` あり (Debug / Development) | `.hlsl` が `Cache/Shaders/` の `.cso` より新しければ実行時にコンパイルし、結果を `.cso` として保存する。コンパイルエラー時は前回成功した `.cso` を使う (エラー内容はログに出力される) |
| `ENABLE_DEV_TOOLS` なし (Release) | コンパイルせず、`Cache/Shaders/` の `.cso` を直接読み込む。見つからなければエラーで終了する |

PSO は使われたときに初めて生成されるため、Release で使うシェーダーの `.cso` をそろえるには、Debug か Development で一度ゲームを起動し、すべての描画が行われるシーンを表示しておく必要があります。

### 5-3. シェーダーのフォルダ

| フォルダ | 内容 |
| :--- | :--- |
| `Common/` | 共通の `.hlsli` (PBR、ライティング、シャドウ、カメラ、定数など) |
| `Objects/` | 3D モデル (インスタンシング、スキニング、アウトライン) |
| `Environment/` | 草・木・地形・水・空 (生成とカリングのコンピュートシェーダーを含む) |
| `Shadow/` | シャドウマップ |
| `PostProcess/` | ポストエフェクト |
| `Volumetric/` | ボリュメトリックフォグ、3D ノイズ生成 |
| `Effects/` | パーティクル、トレイル、雷 |
| `UI/` | スプライト |
| `Debug/` | デバッグライン |

ファイル名は `<名前>.<ステージ>.hlsl` (例: `Grass.VS.hlsl`、`GrassCulling.CS.hlsl`) の規則で付けられています。

---

## 6. キャッシュフォルダ

`Cache/` 以下は自動生成されるファイルです。中身を削除しても、次回の起動時 (開発ビルド) に作り直されます。シェーダーやテクスチャの表示がおかしいときは、`Cache/` を削除して再生成すると解決することがあります。
