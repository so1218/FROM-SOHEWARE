# FromSoheWare Engine 開発ドキュメント

FromSoheWareエンジンを使ったゲーム開発の公式マニュアルです。
目的の項目に合わせて、以下のガイドを参照してください。

## 1. シーンとライフサイクル
ゲーム全体の進行や、オブジェクトを一括管理するシーンの作り方です。
→ **[シーン構築ガイド (sceneGuide.md)](sceneGuide.md)**
- シーンの作成と `BaseScene` の継承
- オブジェクトの自動ライフサイクル (Init ➔ Update ➔ Draw)
- カメラやマネージャーの登録
- `Game` クラスへのシーン登録と遷移

## 2. オブジェクトの作成と制御
実際にゲーム内で動くキャラクターや背景などを定義する `GameObject` の使い方です。
→ **[GameObject 開発ガイド (gameObjectGuide.md)](gameObjectGuide.md)**
- `GameObject` クラスの基本仕様 (Tag, Priority, State)
- キャラクタークラス (`Player` 等) の実装例
- `Collider` を使った衝突判定のセットアップ

## 3. 描画
モデルの表示などの描画機能の使い方です。
→ **[描画ガイド (drawingGuide.md)](drawingGuide.md)**
- `Model`, `AnimationModel` の再生

## 4. デバッグとパラメータ調整
ゲームを実行しながらパラメータを調整するインゲームエディタの使い方です。
→ **[プロパティバインダー ガイド (propertyBinderGuide.md)](propertyBinderGuide.md)**
- `PropertyBinder` による変数と ImGui の自動連携
- パラメータの保存とロード