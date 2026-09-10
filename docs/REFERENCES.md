# 参考文献・使用技術リンク集 (References)

本プログラムのグラフィックスアルゴリズムや物理シミュレーションの実装にあたり、参考にした文献・論文・技術記事の一覧です。

## ボリュメトリックフォグ (Volumetric Fog)

### Froxel（Voxel Grid）ベース・フォグ構造
- **参考資料**: [Volumetric Fog: Unified compute shader based solution to atmospheric scattering](https://www.bartwronski.com/publications/)
  - 著者: Bart Wronski (EA Frostbite / SIGGRAPH 2014)
  - 参考解説記事: [Froxelを用いたボリューメトリックフォグの実装](https://zenn.dev/inpro/articles/73bc866b0d5d15) (Zenn)
- **該当ソースコード**: `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 視界空間を対数分割した3D Voxel（Froxel）格子に対し、ライトの散乱と減衰をコンピュートシェーダーで書き込む、統合フォグの基礎構造。

### 大気レンダリング・フォグ制御
- **参考資料**: [The Weather of Red Dead Redemption 2](https://www.advances.realtimerendering.com/s2019/index.htm)
  - 著者: Fabien Christensen, Alexandre Roche (Rockstar Games / SIGGRAPH 2019)
- **該当ソースコード**: `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 大規模オープンワールドにおける高度フォグ減衰、カスケードシャドウ（CSM）からの遮蔽計算、およびBox/Sphereのトランスフォーム行列を利用した可変密度ローカルフォグボリュームの合成アルゴリズム。

### Interleaved Gradient Noise (IGN) によるレイマーチング・ジッタリング
- **参考資料**: [Next Generation Post Processing in Call of Duty: Advanced Warfare](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)
  - 著者: Jorge Jimenez (Activision / SIGGRAPH 2014)
  - アルゴリズム解説: [Interleaved Gradient Noise: A Different Kind of Low Discrepancy Sequence](https://blog.demofox.org/2022/01/01/interleaved-gradient-noise-a-different-kind-of-low-discrepancy-sequence/) (Demofox)
- **該当ソースコード**: `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 黄金比とフレームインデックスを用いた高効率なスクリーン空間ノイズ。Froxelのスライス境目に生じるディザリング・アーティファクトを散逸させ、後のTemporal Filter等で平滑化する目的で使用。

### 二重 Henyey-Greenstein 位相関数 (Dual-Phase HG Function)
- **参考資料**: [Production Volume Rendering (SIGGRAPH 2017 Course)](https://dl.acm.org/doi/10.1145/3084873.3084907)
  - 著者: Julian Fong, Magnus Wrenninge et al. (Pixar / Weta Digital)
  - 閲覧用ミラー: [Production Volume Rendering: Fundamentals](https://www.researchgate.net/publication/284704974_Production_Volume_Rendering_Fundamentals) (ResearchGate)
- **該当ソースコード**: `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 前方散乱と後方散乱を線形補間で合成し、逆光時の強い光の輪（ハロー効果）と順光時の自然な明るさの双方を物理ベースに表現。

### Perlin-Worley 複合 3D ノイズ (3D Noise Texture Generation)
- **参考資料**: [The Real-time Volumetric Cloudscapes of Horizon Zero Dawn](http://advances.realtimerendering.com/s2015/index.html) 
  - 著者: Andrew Schneider (Guerrilla Games / SIGGRAPH 2015)
  - 閲覧用ミラー: [スライド資料](https://www.slideshare.net/slideshow/the-realtime-volumetric-cloudscapes-of-horizon-zero-dawn/51996465)
- **該当ソースコード**: `Assets/Shaders/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: Rチャンネルに形状を定義するPerlin fBmノイズ、G/B/Aチャンネルに解像度の異なるWorleyノイズ（浸食・ディテール用）を焼き込んだ3D RGBAテクスチャを生成。レイマーチング時のサンプリング負荷を最適化。

### Domain Warping（ドメイン・ワーピング）
- **参考資料**: [domain warping](https://iquilezles.org/articles/warp/) (Inigo Quilez)
- **該当ソースコード**: `Assets/Shaders/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: ノイズ計算の入力座標自体を別のノイズで歪ませることで、直線的なパターンを崩し、流体や煙のような有機的なうねりを表現。

### Worley Noise (Cellular Noise)
- **参考資料**: [A Cellular Texture Basis Function](https://dl.acm.org/doi/10.1145/237170.237267)
  - 著者: Steven Worley (SIGGRAPH 1996)
  - アルゴリズム解説: [The Book of Shaders - セルラーノイズ](https://thebookofshaders.com/12/?lan=jp)
- **該当ソースコード**: `Assets/Shaders/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 格子点からの距離関数によるセル状ノイズ。値を反転させることで、雲のモコモコとした球状感や細かなちぎれエッジを合成。

## 水(Water)

### Gerstner Wave（頂点波形計算）
- **参考資料**: [GPU Gems Chapter 1: Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models) (NVIDIA)
- **該当ソースコード**: `Assets/Shaders/Water.VS.hlsl`
- **概要**: 複数の正弦波を重ね合わせ、頂点を水平・垂直方向に変位させて自然な波頭を表現。

### Reoriented Normal Mapping
- **参考資料**: [Blending in Detail](https://blog.selfshadow.com/publications/blending-in-detail/) (Colin Barré-Brisebois)
- **該当ソースコード**: `Assets/Shaders/Water.PS.hlsl`
- **概要**: 詳細感を損なわずに複数の法線マップを傾きに応じて直交合成。

### リアルタイム水面干渉 (Dynamic Interaction Map)
- **参考資料**: [GPU Gems 2 Chapter 18: Using Vertex Texture Displacement for Realistic Water Rendering](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-18-using-vertex-texture-displacement)
  - 著者: Yuri Kryachko (1C:Maddox Games)
- **該当ソースコード**: `Assets/Shaders/Water.PS.hlsl`
- **概要**: オブジェクトの位置や衝撃波をレンダーターゲットに書き込み、頂点変位（Vertex Texture Fetch）や押し出し法線・波紋として動的に適用。