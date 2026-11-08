# API リファレンス

C++ ヘッダーから Doxygen で自動生成したクラスリファレンスです。モジュール (フォルダ) ごとにページを分けています。

:::{note}
ヘッダーのコメントは `//` 形式のため、Doxygen は説明文を拾いません。各ページの一覧表の「説明」はヘッダー内でクラス直前に書かれた `//` コメントを転記したものです。
メンバー関数の説明を表示したい場合は、ヘッダーのコメントを `///` (または `/** */`) 形式で書いてください。次のビルドから自動で反映されます。
:::

| ページ | 対象フォルダ | クラス/構造体数 |
| :--- | :--- | ---: |
| [コア (Engine / Core)](core.md) | `Engine/Engine.h`, `Engine/Core/` | 8 |
| [フレームワーク (Framework)](framework.md) | `Engine/Framework/` | 16 |
| [カメラ (Camera)](camera.md) | `Engine/Camera/` | 4 |
| [衝突判定 (Collision)](collision.md) | `Engine/Collision/` | 5 |
| [グラフィックス: デバイス (Graphics/Device, Utility)](graphics-device.md) | `Engine/Graphics/Device/`, `Engine/Graphics/Utility/`, `Engine/Graphics/Common/` | 7 |
| [グラフィックス: リソース (Graphics/Resource)](graphics-resource.md) | `Engine/Graphics/Resource/` | 26 |
| [グラフィックス: 描画システム (Graphics/RenderSystem)](graphics-rendersystem.md) | `Engine/Graphics/RenderSystem/` | 28 |
| [グラフィックス: レンダラー (Graphics/Renderer)](graphics-renderer.md) | `Engine/Graphics/Renderer/` | 21 |
| [グラフィックス: ポストエフェクト (Graphics/PostProcess)](graphics-postprocess.md) | `Engine/Graphics/PostProcess/` | 14 |
| [グラフィックス: パーティクル (Graphics/Particle)](graphics-particle.md) | `Engine/Graphics/Particle/` | 20 |
| [アニメーション・メッシュ (Graphics/Animation, Mesh)](graphics-animation.md) | `Engine/Graphics/Animation/`, `Engine/Mesh/` | 13 |
| [数学 (Math)](math.md) | `Engine/Math/` | 12 |
| [オーディオ (Audio)](audio.md) | `Engine/Audio/` | 6 |
| [リソースマネージャー (Resources)](resources.md) | `Engine/Resources/` | 5 |
| [デバッグ (Debugs, Effect)](debug.md) | `Engine/Debugs/`, `Engine/Effect/` | 8 |
| [ゲーム側 (Game)](game.md) | `Game/` | 34 |

```{toctree}
:maxdepth: 1
:hidden:

core
framework
camera
collision
graphics-device
graphics-resource
graphics-rendersystem
graphics-renderer
graphics-postprocess
graphics-particle
graphics-animation
math
audio
resources
debug
game
```
