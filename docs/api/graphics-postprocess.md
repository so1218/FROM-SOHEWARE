# グラフィックス: ポストエフェクト (Graphics/PostProcess)

SSAO・ブルーム・被写界深度・ボリュメトリックフォグ・最終合成などのポストエフェクトパスです。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::BilateralBlurPass <fs-bilateralblurpass>` | class | `Engine/Graphics/PostProcess/BilateralBlurPass.h` |  |
| {ref}`FS::BloomPass <fs-bloompass>` | class | `Engine/Graphics/PostProcess/BloomPass.h` |  |
| {ref}`FS::BrightExtractPass <fs-brightextractpass>` | class | `Engine/Graphics/PostProcess/BrightExtractPass.h` |  |
| {ref}`FS::DoFPass <fs-dofpass>` | class | `Engine/Graphics/PostProcess/DoFPass.h` |  |
| {ref}`FS::FinalCompositePass <fs-finalcompositepass>` | class | `Engine/Graphics/PostProcess/FinalCompositePass.h` |  |
| {ref}`FS::IPostEffect <fs-iposteffect>` | class | `Engine/Graphics/PostProcess/IPostEffect.h` | ポストエフェクト基底クラス |
| {ref}`FS::GeneratedTextureData <fs-generatedtexturedata>` | struct | `Engine/Graphics/PostProcess/NoiseTextureGenerator.h` | 生成したテクスチャとそのSRVインデックスを保持する構造体 |
| {ref}`FS::NoiseTextureGenerator <fs-noisetexturegenerator>` | class | `Engine/Graphics/PostProcess/NoiseTextureGenerator.h` |  |
| {ref}`FS::PostEffectContext <fs-posteffectcontext>` | struct | `Engine/Graphics/PostProcess/PostEffectContext.h` |  |
| {ref}`FS::PostEffectManager <fs-posteffectmanager>` | class | `Engine/Graphics/PostProcess/PostEffectManager.h` |  |
| {ref}`FS::SSAOPass <fs-ssaopass>` | class | `Engine/Graphics/PostProcess/SSAOPass.h` |  |
| {ref}`FS::VolumetricFogBilateralPass <fs-volumetricfogbilateralpass>` | class | `Engine/Graphics/PostProcess/VolumetricFogBilateralPass.h` |  |
| {ref}`FS::VolumetricFogPass <fs-volumetricfogpass>` | class | `Engine/Graphics/PostProcess/VolumetricFogPass.h` |  |
| {ref}`FS::WorldInteractionPass <fs-worldinteractionpass>` | class | `Engine/Graphics/PostProcess/WorldInteractionPass.h` |  |

(fs-bilateralblurpass)=
## FS::BilateralBlurPass

ヘッダー: `Engine/Graphics/PostProcess/BilateralBlurPass.h`

```{doxygenclass} FS::BilateralBlurPass
:members:
:protected-members:
:undoc-members:
```

(fs-bloompass)=
## FS::BloomPass

ヘッダー: `Engine/Graphics/PostProcess/BloomPass.h`

```{doxygenclass} FS::BloomPass
:members:
:protected-members:
:undoc-members:
```

(fs-brightextractpass)=
## FS::BrightExtractPass

ヘッダー: `Engine/Graphics/PostProcess/BrightExtractPass.h`

```{doxygenclass} FS::BrightExtractPass
:members:
:protected-members:
:undoc-members:
```

(fs-dofpass)=
## FS::DoFPass

ヘッダー: `Engine/Graphics/PostProcess/DoFPass.h`

```{doxygenclass} FS::DoFPass
:members:
:protected-members:
:undoc-members:
```

(fs-finalcompositepass)=
## FS::FinalCompositePass

ヘッダー: `Engine/Graphics/PostProcess/FinalCompositePass.h`

```{doxygenclass} FS::FinalCompositePass
:members:
:protected-members:
:undoc-members:
```

(fs-iposteffect)=
## FS::IPostEffect

ポストエフェクト基底クラス

ヘッダー: `Engine/Graphics/PostProcess/IPostEffect.h`

```{doxygenclass} FS::IPostEffect
:members:
:protected-members:
:undoc-members:
```

(fs-generatedtexturedata)=
## FS::GeneratedTextureData

生成したテクスチャとそのSRVインデックスを保持する構造体

ヘッダー: `Engine/Graphics/PostProcess/NoiseTextureGenerator.h`

```{doxygenstruct} FS::GeneratedTextureData
:members:
:protected-members:
:undoc-members:
```

(fs-noisetexturegenerator)=
## FS::NoiseTextureGenerator

ヘッダー: `Engine/Graphics/PostProcess/NoiseTextureGenerator.h`

```{doxygenclass} FS::NoiseTextureGenerator
:members:
:protected-members:
:undoc-members:
```

(fs-posteffectcontext)=
## FS::PostEffectContext

ヘッダー: `Engine/Graphics/PostProcess/PostEffectContext.h`

```{doxygenstruct} FS::PostEffectContext
:members:
:protected-members:
:undoc-members:
```

(fs-posteffectmanager)=
## FS::PostEffectManager

ヘッダー: `Engine/Graphics/PostProcess/PostEffectManager.h`

```{doxygenclass} FS::PostEffectManager
:members:
:protected-members:
:undoc-members:
```

(fs-ssaopass)=
## FS::SSAOPass

ヘッダー: `Engine/Graphics/PostProcess/SSAOPass.h`

```{doxygenclass} FS::SSAOPass
:members:
:protected-members:
:undoc-members:
```

(fs-volumetricfogbilateralpass)=
## FS::VolumetricFogBilateralPass

ヘッダー: `Engine/Graphics/PostProcess/VolumetricFogBilateralPass.h`

```{doxygenclass} FS::VolumetricFogBilateralPass
:members:
:protected-members:
:undoc-members:
```

(fs-volumetricfogpass)=
## FS::VolumetricFogPass

ヘッダー: `Engine/Graphics/PostProcess/VolumetricFogPass.h`

```{doxygenclass} FS::VolumetricFogPass
:members:
:protected-members:
:undoc-members:
```

(fs-worldinteractionpass)=
## FS::WorldInteractionPass

ヘッダー: `Engine/Graphics/PostProcess/WorldInteractionPass.h`

```{doxygenclass} FS::WorldInteractionPass
:members:
:protected-members:
:undoc-members:
```
