[![DebugBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/so1218/FROM-SOHEWARE/actions/workflows/DevelopmentBuild.yml)

<!-- 【TODO: docs/images/hero.png など、デモ画面やシーンエディタのスクリーンショットを配置】 -->
![Engine Demo Header](docs/images/hero.png)

## 概要 
**FromSoheWare** は、DirectX 12 と C++ を使って作っている自作ゲームエンジンです。

グラフィックス API 特有の複雑な初期化や描画処理を使いやすくまとめ、手軽にグラフィックスプログラミングを楽しめる環境を目指して開発しています。

---

## 主なポイント 

- 🎨 **手軽な描画管理**: DirectX 12 の定数バッファやパイプライン状態（PSO）を分かりやすく整理
- 📦 **アセットの読み込み**: 
  - **Assimp** を使って 3D モデル（`.fbx`, `.obj` など）をスムーズにインポート
  - **DirectXTex** を使って各種テクスチャ（`.dds`, `.png`, `.jpg` など）を自動でロード
- 🛠️ **便利なお助けツール**: **Dear ImGui** を組み込んでおり、実行中にパラメータをいじったり状況を確認したりできます
- ⚡ **すっきりした設計**: メインの名前空間 `FSW` のもと、機能ごとにコードをきれいに分けています

---

## 使っている技術・ライブラリ

| カテゴリ | 内容 |
| :--- | :--- |
| **使用言語** | C++17 / C++20 |
| **グラフィックス** | DirectX 12 (Direct3D 12) / HLSL |
| **開発環境** | Visual Studio 2022 (Windows 10 / 11) |
| **ライブラリ** | DirectXTex, Assimp, Dear ImGui, WinPixEventRuntime |
| **自動テスト (CI)** | GitHub Actions |

---

## ビルド構成 (目的別の設定)

状況に合わせて 3 つのモードを使い分けられるようにしています。

| 構成 | 特徴と使いどころ |
| :--- | :--- |
| **Debug** | **詳細な調査用**<br>DirectX 12 のデバッグ機能を有効にし、エラーやAPIの使い方の間違いをしっかりチェックします。 |
| **Development** | **普段の開発用**<br>インゲームエディタや ImGui、検証用テストシーンがそのまま使えます。動作も軽いので普段の作業に最適です。 |
| **Release** | **最終チェック・配布用**<br>デバッグ用UIやログ機能をオフにし、最高の動作スピードで実行します。 |

---

## 試してみる (クイックスタート)

### 必要な環境

| 項目 | 条件 |
| :--- | :--- |
| **OS** | Windows 10 または Windows 11 |
| **開発ツール** | Visual Studio 2022 |
| **グラフィック** | DirectX 12 に対応したグラフィックボード |

### 動かし方

1. **リポジトリを持ってくる**
   ```bash
   git clone --recursive [https://github.com/so1218/FROM-SOHEWARE.git](https://github.com/so1218/FROM-SOHEWARE.git)
   cd FROM-SOHEWARE