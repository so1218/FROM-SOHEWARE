# 変更履歴

版ごとの変更点です。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## v0.9.1 (2026-11-09)

### 変更
- 新しいゲームを始める手順に、スタートアップ プロジェクトの設定を追加
- Visual Studio で開いたときに `Engine.vcxitems.filters` が書き換わらないようにした
- `Engine/.gitattributes` を追加し、取り込んだリポジトリの設定によらず改行コードをそろえるようにした
- ひな形の `.gitignore` で `imgui_layout.ini` を無視するようにした

### 対応が必要
- すでに始めたゲームは、自分の `.gitignore` に `imgui_layout.ini` を足す (ウィンドウの配置を人ごとに変えたいとき)

## v0.9.0 (2026-11-09)

### 追加
- エンジンを指定した版に更新するスクリプト (`UpdateEngine.ps1`)
- 新しいゲームのひな形 (`Template/`) と、エンジンに必要なビルド設定 (`Engine.props`)
- エンジンのビルドでできるファイルを無視する `.gitignore`
- ドキュメント (`Docs/`)。新しいゲームの始め方 (`Docs/start/new-project.md`) を含む
- `Assets/Data/FontList.csv` に書いたフォントが `Assets/Data/Fonts/` にないとき、`Engine/Assets/Fonts/` から探す

### 変更
- Release に `ENABLE_IMGUI`・`ENABLE_DEBUG_DRAW`・`ENABLE_DEBUG_CAMERA` を入れないようにした。Release でも使いたいゲームは、自分の vcxproj の Release に足す
- シェーダーを `Engine/Shaders/`、外部ライブラリを `Engine/externals/`、エンジンが使うアセットを `Engine/Assets/` に移動
