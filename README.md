[![DebugBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DevelopmentBuild.yml)

![Engine Demo Header](docs/Textures/scene_01.png)

## 概要 
**FromSoheWare**（`FSW`）は、DirectX 12 と C++ を用いて構築された、高品質な自然環境や広大な世界観の表現を目指す自作 3D ゲームエンジンです。

グラフィックス API 特有の複雑な初期化・描画処理を抽象化しつつ、ボリュメトリックフォグ等のグラフィックス技術と、直感的なパラメータ調整が可能なインゲーム開発ツールを統合しています。

---

## 主な技術的強み 

### 1. ボリュメトリックフォグ (Volumetric Fog)
- **視錐台ボクセル（Froxel）サンプリング**: 視錐台をボクセル状に分割して計算量を大幅に削減し、高精度な霧表現を低負荷で実現。
- **ライトシャフト**: Directional Light / Point Light / Spot Light から漏れ出る光の筋（ライトレイ）をリアルタイム計算。
- **ボリュームフォグ**: 空間全体のフォグに加え、Box 型および Sphere 型のローカルフォグ配置に対応。
- **3Dノイズ連携**: 単純な距離フォグではなく、ノイズテクスチャによる空気の揺らぎや密度の濃淡を表現。

### 2. 実ゲームを見据えたパフォーマンス最適化 (60 FPS 維持)
- **カリング処理**: 視錐台カリングおよび距離カリングにより、遠景の草木や 3D モデルの不要な GPU 描画を自動カット。
- **PIX プロファイリング & パスマーカー**: `WinPixEventRuntime` を統合し、描画パスごとのプロファイリングと可視化を実施。多数のパスを通しながらも **GPU 時間約 8ms（60 FPS 安定）** を達成。

### 3. データ保存とUI化を一括で行う `PropertyBinder`
- **僅か 2 行でのデータバインド**: 変数登録と描画呼び出しの最小コードで、JSON への自動保存/読み込みと Dear ImGui でのデバッグ UI 表示を同期。
- **マルチマテリアルリアルタイム調整**: モデル単位・マテリアル単位でカラー、法線マップ、粗さ、リムライト等のパラメータを画面上で即座に調整可能。

### 4. カスタムパーティクルシステム & インゲームエディタ
- **モジュール構造**: Unity のパーティクルシステムを参考に設計。エミッターを作成・追加し、インゲームのパーティクルエディタ上でパラメータを調整しながら各種エフェクトを制作可能。

---

## 描画・オブジェクト設計

- **描画クラス**: `Model`, `AnimationModel`, `Sprite`, `Line` などの基本描画クラスを完備。
- **ライフサイクル自動化**: `GameObject` を継承したオブジェクトをシーンに登録するだけで、初期化・更新（Update）・描画（Draw）のライフサイクルが自動で周回。

---

## 技術スタック & 開発ツール

| カテゴリ | 内容 / 使用ツール |
| :--- | :--- |
| **使用言語** | C++17 / C++20 |
| **グラフィックス API** | DirectX 12 / HLSL |
| **プロファイリング・デバッグ** | PIX for Windows (`WinPixEventRuntime`), Dear ImGui |
| **外部ライブラリ** | DirectXTex, Assimp |
| **アセット制作・編集** | Blender, Mixamo, EffectTextureMaker |
| **自動テスト (CI)** | GitHub Actions |

---

## 使用アセット

本エンジンのデモ・検証シーン作成において、以下の素材およびツールを活用させていただいています。

- **3D モデル / アニメーション**: [Sketchfab](https://sketchfab.com/), [Mixamo](https://www.mixamo.com/)
- **テクスチャ / 環境マップ (HDRI)**: [Poly Haven](https://polyhaven.com/)
- **エフェクトテクスチャ作成**: EffectTextureMaker

---

## 📚 参考資料

グラフィックスパイプラインおよびエンジン機能の構築にあたり、以下の技術資料や商用エンジンを参考に設計を行いました。

- **レンダリング技術・ポストエフェクト**
  - SIGGRAPH / GDC 講演論文
  - NVIDIA Developer Graphics Articles
  - Zenn (技術コミュニティ記事)
- **エンジン設計・アーキテクチャ**
  - **Unity**: モジュール型パーティクルシステムの設計思想
  - **Unreal Engine**: Property Binding（プロパティ結合）およびマテリアルパラメータ調整構造

---

## ビルド構成 

| 構成 | 特徴と使いどころ |
| :--- | :--- |
| **Debug** | **詳細なデバッグ用**<br>DirectX 12 のデバッグレイヤーを有効にし、API の誤用や警告を詳細にチェック。 |
| **Development** | **普段の開発用 (おすすめ)**<br>インゲームエディタや Dear ImGui、検証用テストシーンが使用可能。動作も軽量で開発作業に最適。 |
| **Release** | **最終チェック・配布用**<br>デバッグ用 UI やログ機能をオフにし、最高速度で実行。 |

---

## クイックスタート

### 動作環境 

| 項目 | 条件 |
| :--- | :--- |
| **OS** | Windows 10 または Windows 11 (64-bit) |
| **開発ツール** | Visual Studio 2022 (「C++ によるデスクトップ開発」ワークロード必須) |
| **グラフィック** | DirectX 12 (Feature Level 11_0 以上) に対応した GPU |

### ビルド手順

1. **リポジトリのクローン**
   ```bash
   git clone --recursive [https://github.com/so1218/FROM-SOHEWARE.git](https://github.com/so1218/FROM-SOHEWARE.git)
   cd FROM-SOHEWARE