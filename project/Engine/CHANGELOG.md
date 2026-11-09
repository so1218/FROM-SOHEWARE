# 変更履歴

版ごとの変更点。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## 未リリース

### 追加
- ひな形に README を追加。GitHub でゲームのリポジトリを開くと、エンジンのドキュメントへのリンクが出る。すでに始めたゲームは `Engine/Template/README.md` を一番上にコピーして使える

## v0.9.3

## v0.9.2

### 追加
- Release をビルドすると、配布できる形のフォルダを出力先の `Package/` に作るようにした (`Docs/guides/packaging.md`)

### 変更
- Development でコンパイルするシェーダーを最適化するようにした。Release と完成版も、最適化したシェーダーを使う。Debug のシェーダーは最適化せず `Cache/ShadersDebug/` に保存する
- `UpdateEngine.ps1` が、Git にまだ登録していない新しいファイルがあるだけで止まらないようにした
- 更新の取り消しは、前の版を指定して `UpdateEngine.ps1` を実行する方法にした (`git revert` で戻すと、同じ版を取り込み直せなくなるため)
- すでに取り込んである版を指定したときに、「何も変わらなかった」と表示するようにした

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
