# FromSoheWare Engine 開発ドキュメント

FromSoheWareエンジンを使ったゲーム開発のマニュアルです。
開発を始める際は、以下のガイドを参照してください。

## 1. エンジンの基本設計と共通仕様
エンジン全体の設計思想や、実装時に知っておくべき共通ルールについての解説です。
→ **[エンジン基本設計ガイド (engineBasicsGuide.md)](engineBasicsGuide.md)**
- エンジンの基盤構造と `Engine*` ポインタの受け渡し規則
- プリコンパイル済みヘッダー (`pch.h`) の役割と使い方
- アプリケーションのライフサイクル (起動 ➔ 初期化 ➔ メインループ ➔ 破棄)
- 共通ユーティリティ（時間管理 `TimeManager` 等）へのアクセス

## 2. シーンとライフサイクル
ゲーム全体の進行や、オブジェクトを一括管理するシーンの作り方です。
→ **[シーン構築ガイド (sceneGuide.md)](sceneGuide.md)**
- シーンの作成と `BaseScene` の継承
- オブジェクトの自動ライフサイクル (Init ➔ Update ➔ Draw)
- カメラやマネージャーの登録
- `Game` クラスへのシーン登録と遷移

## 3. オブジェクトの作成と制御
実際にゲーム内で動くキャラクターや背景などを定義する `GameObject` の使い方です。
→ **[GameObject 開発ガイド (gameObjectGuide.md)](gameObjectGuide.md)**
- `GameObject` クラスの基本仕様 (Tag, Priority, State)
- キャラクタークラス (`Player` 等) の実装例
- `Collider` を使った衝突判定のセットアップ

## 4. 描画
モデルや2D要素の表示など、グラフィックス機能の使い方です。
→ **[描画ガイド (drawingGuide.md)](drawingGuide.md)**
- `Model`, `AnimationModel` の再生
- `Sprite` (2D/UI), `Line` (3D直線) の描画と制御

## 5. デバッグとパラメータ調整
ゲームを実行しながらパラメータを調整するインゲームエディタの使い方です。
→ **[プロパティバインダー ガイド (propertyBinderGuide.md)](propertyBinderGuide.md)**
- `PropertyBinder` による変数と ImGui の自動連携
- パラメータの保存とロード (JSON)