# 参考文献・使用技術リンク集 (References)

本プログラムのグラフィックスアルゴリズムや物理シミュレーションの実装にあたり、参考にした文献・論文・技術記事の一覧です。

## 物理ベースレンダリング (PBR & Lighting Pipeline)

### Cook-Torrance BRDF
- **参考資料**: 
  - [Real Shading in Unreal Engine 4 (SIGGRAPH 2013 Course Notes)](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf) (Brian Karis / Epic Games)
  - [Microfacet Models for Refraction through Rough Surfaces](https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html) (Bruce Walter et al.)
- **該当ソースコード**: `Assets/Shaders/Common/PBRUtils.hlsli`
- **概要**: Cook-TorranceスペキュラBRDFモデルの実装。法線分布関数に **GGX**、幾何遮蔽関数に **Smith joint (Schlick-GGX)**、フレネル反射に **Schlick近似** を採用。エネルギー保存則に基づき、アルベドとメタリック（金属度）から拡散反射（kD）と鏡面反射（kS）を分離して物理的に正しく合成する計算パイプライン。

## ボリュメトリックフォグ (Volumetric Fog)

### Froxel（Voxel Grid）ベース・フォグ構造
- **参考資料**: [Volumetric Fog: Unified compute shader based solution to atmospheric scattering](https://www.bartwronski.com/publications/)
  - 著者: Bart Wronski (EA Frostbite / SIGGRAPH 2014)
  - 参考解説記事: [Froxelを用いたボリューメトリックフォグの実装](https://zenn.dev/inpro/articles/73bc866b0d5d15) (Zenn)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VolumetricFogInjection.CS.hlsl`
- **概要**: 視界空間を対数分割した3D Voxel（Froxel）格子に対し、ライトの散乱と減衰をコンピュートシェーダーで書き込む、統合フォグの基礎構造。

### 大気レンダリング・フォグ制御
- **参考資料**: [The Weather of Red Dead Redemption 2](https://www.advances.realtimerendering.com/s2019/index.htm)
  - 著者: Fabien Christensen, Alexandre Roche (Rockstar Games / SIGGRAPH 2019)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VolumetricFogInjection.CS.hlsl`
- **概要**: 大規模オープンワールドにおける高度フォグ減衰、カスケードシャドウ（CSM）からの遮蔽計算、およびBox/Sphereのトランスフォーム行列を利用した可変密度ローカルフォグボリュームの合成アルゴリズム。

### Interleaved Gradient Noise (IGN) によるレイマーチング・ジッタリング
- **参考資料**: [Next Generation Post Processing in Call of Duty: Advanced Warfare](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)
  - 著者: Jorge Jimenez (Activision / SIGGRAPH 2014)
  - アルゴリズム解説: [Interleaved Gradient Noise: A Different Kind of Low Discrepancy Sequence](https://blog.demofox.org/2022/01/01/interleaved-gradient-noise-a-different-kind-of-low-discrepancy-sequence/) (Demofox)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VolumetricFogInjection.CS.hlsl`
- **概要**: 黄金比とフレームインデックスを用いた高効率なスクリーン空間ノイズ。Froxelのスライス境目に生じるディザリング・アーティファクトを散逸させ、後のTemporal Filter等で平滑化する目的で使用。

### 二重 Henyey-Greenstein 位相関数 (Dual-Phase HG Function)
- **参考資料**: [Production Volume Rendering (SIGGRAPH 2017 Course)](https://dl.acm.org/doi/10.1145/3084873.3084907)
  - 著者: Julian Fong, Magnus Wrenninge et al. (Pixar / Weta Digital)
  - 閲覧用ミラー: [Production Volume Rendering: Fundamentals](https://www.researchgate.net/publication/284704974_Production_Volume_Rendering_Fundamentals) (ResearchGate)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VolumetricFogInjection.CS.hlsl`
- **概要**: 前方散乱と後方散乱を線形補間で合成し、逆光時の強い光の輪（ハロー効果）と順光時の自然な明るさの双方を物理ベースに表現。

### Perlin-Worley 複合 3D ノイズ (3D Noise Texture Generation)
- **参考資料**: [The Real-time Volumetric Cloudscapes of Horizon Zero Dawn](http://advances.realtimerendering.com/s2015/index.html) 
  - 著者: Andrew Schneider (Guerrilla Games / SIGGRAPH 2015)
  - 閲覧用ミラー: [スライド資料](https://www.slideshare.net/slideshow/the-realtime-volumetric-cloudscapes-of-horizon-zero-dawn/51996465)
- **該当ソースコード**: `Assets/Shaders/Volumetric/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: Rチャンネルに形状を定義するPerlin fBmノイズ、G/B/Aチャンネルに解像度の異なるWorleyノイズ（浸食・ディテール用）を焼き込んだ3D RGBAテクスチャを生成。レイマーチング時のサンプリング負荷を最適化。

### Domain Warping（ドメイン・ワーピング）
- **参考資料**: [domain warping](https://iquilezles.org/articles/warp/) (Inigo Quilez)
- **該当ソースコード**: `Assets/Shaders/Volumetric/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: ノイズ計算の入力座標自体を別のノイズで歪ませることで、直線的なパターンを崩し、流体や煙のような有機的なうねりを表現。

### Worley Noise (Cellular Noise)
- **参考資料**: [A Cellular Texture Basis Function](https://dl.acm.org/doi/10.1145/237170.237267)
  - 著者: Steven Worley (SIGGRAPH 1996)
  - アルゴリズム解説: [The Book of Shaders - セルラーノイズ](https://thebookofshaders.com/12/?lan=jp)
- **該当ソースコード**: `Assets/Shaders/Volumetric/Generate3DNoise.CS.hlsl`, `Assets/Shaders/VolumetricFogInjection.CS.hlsl`
- **概要**: 格子点からの距離関数によるセル状ノイズ。値を反転させることで、雲のモコモコとした球状感や細かなちぎれエッジを合成。

### Froxel Temporal Reprojection (ボクセル空間の時間軸再投影)
- **参考資料**: [Volumetric Fog: Unified compute shader based solution to atmospheric scattering](https://www.bartwronski.com/publications/)
  - 著者: Bart Wronski (EA Frostbite / SIGGRAPH 2014)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VoxelTemporalResolve.CS.hlsl`
- **概要**: 前フレームのカメラビュープロジェクション行列（`prevViewProj`）を用いて、現在のボクセル位置（ワールド座標）に対する過去フレームの参照位置（`prevUVW`）を逆算（Reprojection）し、時間軸上でフレーム間フリッカーを平滑化。

### Variance Clipping によるゴースト抑制
- **参考資料**: [An Area-Efficient Temporal Anti-Aliasing Algorithm](https://developer.download.nvidia.com/GTC/GTC_2016/S6540_Marco_Salvi_Temporal_Antialiasing.pdf)
  - 著者: Marco Salvi (NVIDIA / GDC 2016)
  - 補足参考: [High-Quality Temporal Supersampling](https://advances.realtimerendering.com/s2014/index.html) (Brian Karis / Epic Games, SIGGRAPH 2014)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VoxelTemporalResolve.CS.hlsl`
- **概要**: 3x3x3（27近傍）ボクセルの1次・2次モーメントから平均値と標準偏差を計算し、カラー空間における境界を形成。過去フレームの履歴をその範囲内に強制収束させることで、動体やカメラ移動時に発生する色滲み・残像（ゴースト）を低減。

### 適応的ブレンド
- **参考資料**: [Temporal Antialiasing in Uncharted 4](https://advances.realtimerendering.com/s2016/index.html)
  - 著者: Ke Xu (Naughty Dog / SIGGRAPH 2016)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VoxelTemporalResolve.CS.hlsl`
- **概要**: 現在のボクセル色と過去履歴との差分を検出し、大きな色変化が発生した箇所では新フレームの比率を高めることで、動的オブジェクトの移動や急激な照明変化に対応。

### Froxel 3D 空間バイラテラルフィルタ (3D Voxel Spatial Denoising)
- **参考資料**: [Volumetric Fog: Unified compute shader based solution to atmospheric scattering](https://www.bartwronski.com/publications/)
  - 著者: Bart Wronski (EA Frostbite / SIGGRAPH 2014)
  - 輝度規格: [ITU-R Recommendation BT.709](https://www.itu.int/rec/R-REC-BT.709)
- **該当ソースコード**: `Assets/Shaders/Volumetric/VoxelSpatialFilter.CS.hlsl`
- **概要**: Froxel格子へのライト注入直後（TAA前段）に生じる高周波ノイズ（IGNや点光源サンプリングの粗）を除去する3D空間フィルタ。Rec.709規格の輝度重み付け（`0.2126, 0.7152, 0.0722`）に基づく6近傍差分評価に加え、奥のボクセルほど対数分割で世界空間上のサイズが大きくなる特性に合わせてエッジ感度（`bilateralSensitivity`）を距離に応じて緩和（`smoothstep`）し、遠景の輪郭チラつきを防止。

## 草木・植生システム・小石 (Grass, Foliage & Pebble System)

### プロシージャル草生成 (Procedural Grass Generation & Grid Snapping)
- **参考資料**: 
  - [GDC 2021: Procedural Grass in 'Ghost of Tsushima'](https://www.youtube.com/watch?v=I3132pY8lFA) (Eric Wohllaib / Sucker Punch Productions)
- **該当ソースコード**: 
  - `Assets/Shaders/Environment/GrassGeneration.CS.hlsl`
  - `Assets/Shaders/Environment/FoliageGeneration.CS.hlsl`
  - `Assets/Shaders/Environment/PebbleGeneration.CS.hlsl`
- **概要**: カメラ移動時のチラつき（ポッピング）を防ぐグリッドスナップ処理とHash関数による疑似乱数ジッター配置。ハイトマップ・密度マップを事前サンプリングしたGPU上の動的インスタンスデータ生成。

### GPU-Driven カリング & Wave Intrinsics 最適化 (GPU-Driven Culling)
- **参考資料**: 
  - [GPU-Driven Rendering Pipelines (SIGGRAPH 2015 Course)](https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf) (Sebastien Aaltonen, Marko Haar / SIGGRAPH 2015)
  - [HLSL Shader Model 6.0 Wave Intrinsics](https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics) (Microsoft / DirectXShaderCompiler Official Wiki)
- **該当ソースコード**: 
  - `Assets/Shaders/GrassCulling.CS.hlsl`
  - `Assets/Shaders/FoliageCulling.CS.hlsl`
  - `Assets/Shaders/PebbleCulling.CS.hlsl`
- **概要**: 6つの視錐台平面（Frustum）と距離に基づく球判定カリングを実施。`ExecuteIndirect` 用のアトミックカウントにおいて、`WaveActiveCountBits` や `WavePrefixCountBits` 等の Wave Intrinsics を使用してレーン（Warp/Wavefront）単位でアトミック書き込みを集約し、メモリバスカウント競合を劇的に削減した高性能GPUカリングパイプライン。

### 草頂点アニメーション & ベジェ曲線物理
- **参考資料**: 
  - [GDC 2021: Procedural Grass in 'Ghost of Tsushima'](https://www.youtube.com/watch?v=I3132pY8lFA) (Sucker Punch Productions)
  - [GPU Gems 3 - Chapter 16: Vegetation Rendering Techniques](https://developer.nvidia.com/gpugems/gpugems3/part-iii-scenery-rendering/chapter-16-vegetation-rendering-techniques) (NVIDIA)
- **該当ソースコード**: `Assets/Shaders/Grass.VS.hlsl`
- **概要**: 3次ベジェ曲線を用いた風・インタラクションマップによる草のしなり計算。外力で変形した際に草がゴムのように伸縮するのを防ぐ **長さ保持アルゴリズム**、遠景カリング時の密度低下を補う **動的幅拡大補正**、および遠距離頂点を先端へ集約する **縮退ポリゴンLOD** の実装。

### 植生ライティング & シェーディング
- **参考資料**: 
- [GPU Gems 3 - Chapter 16: Vegetation Procedural Animation and Shading in Crysis](https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-16-vegetation-procedural-animation-and-shading-crysis) (NVIDIA)
  - [GDC 2021: Procedural Grass in 'Ghost of Tsushima'](https://www.youtube.com/watch?v=I3132pY8lFA) (Sucker Punch Productions)
- **該当ソースコード**:   
  - `Assets/Shaders/Grass.PS.hlsl`
  - `Assets/Shaders/Foliage.PS.hlsl`
  - `Assets/Shaders/TreeFoliage.PS.hlsl`
- **概要**: 草特有の高周波な法線ノイズを軽減する **Y軸（Upベクトル）法線ブレンド**、葉の薄さを表現する **視線依存型Subsurface Scattering**、風の強さに連動してハイライトが波打つ **異方性スペックル・突風マスク処理** を組み込んだ専用Foliageシェーディング。

## 雷 (Lightning)

### 稲妻のビジュアル表現
- **参考モチーフ**: 『Red Dead Redemption 2』等の高密度な稲妻ビジュアル
- **参考資料**: 
  - [The Book of Shaders: Random](https://thebookofshaders.com/10/) (Patricio Gonzalez Vivo & Jen Lowe / 擬似乱数・Hashノイズ)
  - [Inigo Quilez: Useful Little Functions](https://iquilezles.org/articles/functions/) (プロシージャル制御・シェーダー関数)
- **該当ソースコード**: `Assets/Shaders/Effects/Lightning.PS.hlsl`, `Assets/Shaders/Effects/Lightning.VS.hlsl`
- **概要**: UV座標に基づく距離計算から中心の芯と外周の発光を独立して算出し、Hashノイズによる時間的明滅とHDR Bloom対応の高輝度Emissiveを組み合わせたプロシージャルな雷。

## 水(Water)

### Gerstner Wave & 複合波形表現
- **参考資料**: 
  - [GPU Gems - Chapter 1: Effective Water Simulation from Physical Models](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models) (Mark Finch / NVIDIA)
- **該当ソースコード**: `Assets/Shaders/Environment/Water.VS.hlsl`
- **概要**: 複数方向の正弦波（Gerstner Wave）を合成し、水面の水平・垂直方向への自然な波頭変位をリアルタイム計算。

### Reoriented Normal Mapping (RNM)
- **参考資料**: 
  - [Blending in Detail](https://blog.selfshadow.com/publications/blending-in-detail/) (Colin Barré-Brisebois / EA Frostbite)
- **該当ソースコード**: `Assets/Shaders/Environment/Water.PS.hlsl`
- **概要**: 異なるスケールや風向アニメーションを持つ波の法線マップを、傾きと詳細感を損なわずに高精度に直交合成（RNM）。

### スクリーンスペース水面反射 (Screen Space Reflection - SSR)
- **参考資料**: 
  - [Efficient GPU Screen-Space Ray Tracing](https://jcgt.org/published/0003/04/04/) (Morgan McGuire, Michael Mara / JCGT)
  - [Stochastic Screen-Space Reflections (SIGGRAPH 2015)](https://h3.gd/stochastic-ssr/) (Tomasz Stachowiak, Yasin Uludağ / EA Frostbite)
- **該当ソースコード**: `Assets/Shaders/Environment/Water.PS.hlsl`
- **概要**: 線形ステップレイキャストと二分探索（Binary Search Refinement）による高精度SSR。画面端フェード処理およびキューブマップ環境光へのスムーズなフォールバック構造の実装。

### ボロノイ・コースティクス & 色収差 (Voronoi Caustics & Dispersion)
- **参考資料**: 
  - [GPU Gems - Chapter 2: Rendering Water Caustics](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-2-rendering-water-caustics) (NVIDIA)
- **該当ソースコード**: `Assets/Shaders/Environment/Water.PS.hlsl`
- **概要**: 4セルボロノイエッジアルゴリズムを用いたプロシージャル水底コースティクス。波法線によるゆがみや水深減衰に加え、RGBチャンネルを微細オフセットさせた波長分散（色収差）表現。

### 水中吸光 & 物理ベース散乱 (Beer-Lambert Absorption & In-Scattering)
- **参考資料**: 
  - [Simulating Ocean Water (SIGGRAPH Course Notes)](https://jtessen.people.clemson.edu/reports/papers_files/coursenotes2004.pdf) (Jerry Tessendorf)
- **該当ソースコード**: `Assets/Shaders/Environment/Water.PS.hlsl`
- **概要**: **Beer-Lambertの法則**（`exp(-depth * absorption)`）に基づく水深に応じた光波の吸光表現（浅瀬〜深海のグラデーション）と、波頭や逆光時に光が分散する水中散乱（In-Scattering）処理。

## インタラクションシステム

### 動的インタラクションマップ & 軌跡フィールド
- **参考資料**: 
  - [GDC 2021: Procedural Grass in 'Ghost of Tsushima'](https://www.youtube.com/watch?v=I3132pY8lFA) (Sucker Punch Productions)
  - [GDC 2017: Between Tech and Art: The Vegetation of 'Horizon Zero Dawn'](https://www.youtube.com/watch?v=I3132pY8lFA) (Guerrilla Games)
- **該当ソースコード**: `Assets/Shaders/Environment/WorldInteraction.CS.hlsl`
- **概要**: 押し出し方向（RGチャンネル）および瞬間力・時間経過痕跡（BAチャンネル）をグリッドテクスチャへ集約出力するCompute Shader。カメラ移動に伴うワールドUV補正、時間経過による軌跡減衰、および進行方向と放射ベクトルの合成による引き波（Wake）表現を備え、植生・水面・雪原が共通参照できる中央集権型干渉パイプライン。

## ブルーム

### Jorge Jimenez 式 13-Tap Downsample & 9-Tap Tent Upsample
- **参考資料**: 
  - [Next Generation Post Processing in Call of Duty: Advanced Warfare](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/) (Jorge Jimenez / SIGGRAPH 2014)
- **該当ソースコード**: `Assets/Shaders/PostProcess/Downsample.PS.hlsl`, `Assets/Shaders/PostProcess/Upsample.PS.hlsl`
- **概要**: 13タップのダウンサンプリング（ボックス・テントフィルタの結合によるサブピクセルのチラつき抑制）と9タップのテントフィルタ（3x3ガウシアン近似）によるピラミッド型フィルタ階層処理。時間の経過による細かなチラつきを大幅に軽減した高品質なブルーム効果を実現。

## 被写界深度 (Depth of Field)

### 黄金角スパイラルサンプリングと玉ボケ強調 
- **参考資料**: 
  - [Fermat's Spiral / Golden Ratio Sampling](https://blog.demofox.org/2023/02/17/fibonacci-word-sampling-a-sorted-golden-ratio-low-discrepancy-sequence/) (Alan Wolfe / 黄金角を用いた均一円盤サンプリング)
  - [Next Generation Post Processing in Call of Duty: Advanced Warfare](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/) (Jorge Jimenez / SIGGRAPH 2014 - ポストプロセスにおけるディスクサンプリングとジッター技法)
- **該当ソースコード**: `Assets/Shaders/PostProcess/DoF.PS.hlsl`
- **概要**: 黄金角に基づくフェルマーの螺旋を用いた均一な円盤サンプリングを実施。ピクセル固有のランダム回転でバンディングを抑制しつつ、輝度抽出によるハイライト強調を合成することで、軽量かつ高品質な写真風の玉ボケ効果を実現。

## スクリーンスペース・アンビエントオクルージョン (SSAO)

### スパイラル半球サンプリング & ハロー抑制
- **参考資料**: 
  - [Finding Next Gen: CryEngine 2 (SIGGRAPH 2007 Course)](https://www.advances.realtimerendering.com/s2007/index.html) (Martin Mittring / Crytek)
  - [Know Your Shader: Screen Space Ambient Occlusion](http://john-chapman-graphics.blogspot.com/2013/01/ssao.html) (John Chapman)
  - [Next-Generation Post-Processing in Call of Duty: Advanced Warfare](http://www.activision.com/company/publications/call-of-duty-advanced-warfare) (Jorge Jimenez / SIGGRAPH 2014)
- **該当ソースコード**: `Assets/Shaders/PostProcess/SSAO.PS.hlsl`
- **概要**: 判定の特異点（NaN）を回避する安全なTBN行列の構築と、半球内スパイラルサンプリングの実装。`InterleavedGradientNoise` を利用した効率的なジッター回転に加えて、距離減衰関数（`smoothstep`）による深度不連続境界のハローアーティファクト抑制、および遠距離フェード・コントラスト補正を備えたスクリーンスペース遮蔽パイプライン。