# デバッグ (Debugs, Effect)

ImGui 連携、`PropertyBinder`、ロガー、デバッグ描画、D3D12 デバッグレイヤー、画面揺れなどです。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::DebugDraw <fs-debugdraw>` | class | `Engine/Debugs/DebugDraw.h` | デバッグ描画 |
| {ref}`FS::DebugGuiManager <fs-debugguimanager>` | class | `Engine/Debugs/DebugGuiManager.h` |  |
| {ref}`FS::DebugLayerManager <fs-debuglayermanager>` | class | `Engine/Debugs/DebugLayerManager.h` |  |
| {ref}`FS::D3DResourceLeakChecker <fs-d3dresourceleakchecker>` | struct | `Engine/Debugs/DebugUtils.h` | プログラム終了時にDXGIのリソースリークをチェックするクラス |
| {ref}`FS::ImGuiManager <fs-imguimanager>` | class | `Engine/Debugs/ImGuiManager.h` |  |
| {ref}`FS::Logger <fs-logger>` | class | `Engine/Debugs/Logger.h` |  |
| {ref}`FS::PropertyBinder <fs-propertybinder>` | class | `Engine/Debugs/PropertyBinder.h` | 変数の登録を行えば、GlobalVariablesの読み書きとImGuiの表示をしてくれる関数 |
| {ref}`FS::ShakeEffect <fs-shakeeffect>` | class | `Engine/Effect/ShakeEffect.h` |  |

(fs-debugdraw)=
## FS::DebugDraw

デバッグ描画

ヘッダー: `Engine/Debugs/DebugDraw.h`

```{doxygenclass} FS::DebugDraw
:members:
:protected-members:
:undoc-members:
```

(fs-debugguimanager)=
## FS::DebugGuiManager

ヘッダー: `Engine/Debugs/DebugGuiManager.h`

```{doxygenclass} FS::DebugGuiManager
:members:
:protected-members:
:undoc-members:
```

(fs-debuglayermanager)=
## FS::DebugLayerManager

ヘッダー: `Engine/Debugs/DebugLayerManager.h`

```{doxygenclass} FS::DebugLayerManager
:members:
:protected-members:
:undoc-members:
```

(fs-d3dresourceleakchecker)=
## FS::D3DResourceLeakChecker

プログラム終了時にDXGIのリソースリークをチェックするクラス

ヘッダー: `Engine/Debugs/DebugUtils.h`

```{doxygenstruct} FS::D3DResourceLeakChecker
:members:
:protected-members:
:undoc-members:
```

(fs-imguimanager)=
## FS::ImGuiManager

ヘッダー: `Engine/Debugs/ImGuiManager.h`

```{doxygenclass} FS::ImGuiManager
:members:
:protected-members:
:undoc-members:
```

(fs-logger)=
## FS::Logger

ヘッダー: `Engine/Debugs/Logger.h`

```{doxygenclass} FS::Logger
:members:
:protected-members:
:undoc-members:
```

(fs-propertybinder)=
## FS::PropertyBinder

変数の登録を行えば、GlobalVariablesの読み書きとImGuiの表示をしてくれる関数

ヘッダー: `Engine/Debugs/PropertyBinder.h`

```{doxygenclass} FS::PropertyBinder
:members:
:protected-members:
:undoc-members:
```

(fs-shakeeffect)=
## FS::ShakeEffect

ヘッダー: `Engine/Effect/ShakeEffect.h`

```{doxygenclass} FS::ShakeEffect
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### FS::LogLevel

ログレベル

ヘッダー: `Engine/Debugs/Logger.h`

```{doxygenenum} FS::LogLevel
```
