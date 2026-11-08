# FromSoheWare Engine 開発ドキュメント

FromSoheWare は、C++20 と DirectX 12 で作られた自作ゲームエンジンです。
このドキュメントは、エンジンを使ってゲームを作るためのマニュアルと、ヘッダーから自動生成した API リファレンスで構成されています。

![FromSoheWare Engine のデモシーン](textures/scene_01.png)

---

## はじめに読むもの

エンジン全体の仕組みと、ゲームを作るときの基本の流れです。上から順に読むことをおすすめします。

1. **[エンジン基本設計ガイド](engineBasicsGuide.md)**
   `Engine*` の受け渡し規則、`pch.h`、名前空間 `FS`、起動から終了までのライフサイクル、シングルトンの一覧、ビルド構成ごとのマクロ。
2. **[シーン構築ガイド](sceneGuide.md)**
   `BaseScene` を継承したシーンの作り方、`Game` クラスへの登録、シーン遷移。
3. **[GameObject 開発ガイド](gameObjectGuide.md)**
   `GameObject` の仕様（タグ・更新優先度・アクティブ状態）、カスタムオブジェクトの作り方、`Collider` による衝突判定。
4. **[描画ガイド](drawingGuide.md)**
   `Model` / `AnimationModel` / `Sprite` / `Line` の表示と制御、ライトの配置。
5. **[プロパティバインダー ガイド](propertyBinderGuide.md)**
   `PropertyBinder` による変数と ImGui の自動連携、パラメータの保存と読み込み (JSON)。

## 機能別リファレンス

6. **[入力・時間・音声・ログ](systemsGuide.md)**
   `Input`、`TimeManager`、`AudioPlayer`、`Logger` の使い方。
7. **[アセットとデータファイル](assetsGuide.md)**
   モデル・テクスチャ・アニメーション・音声の登録方法 (CSV)、PSO 定義 (JSON)、シェーダーのコンパイルとキャッシュ。
8. **[描画パイプライン](renderingPipeline.md)**
   1 フレームの描画手順（シャドウ → G-Buffer → ポストエフェクト → 最終合成）と、主要クラスの役割分担。
9. **[API リファレンス](api/index.md)**
   C++ ヘッダーから Doxygen で自動生成したクラス一覧。

## その他

- **[参考文献・使用技術リンク集](REFERENCES.md)**
- **[このドキュメントのビルド方法](buildingDocs.md)**

```{toctree}
:hidden:
:caption: はじめに読むもの
:maxdepth: 2

engineBasicsGuide
sceneGuide
gameObjectGuide
drawingGuide
propertyBinderGuide
```

```{toctree}
:hidden:
:caption: 機能別リファレンス
:maxdepth: 2

systemsGuide
assetsGuide
renderingPipeline
api/index
```

```{toctree}
:hidden:
:caption: その他
:maxdepth: 1

REFERENCES
buildingDocs
```
