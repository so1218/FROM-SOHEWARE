# 新しいゲームを始める

エンジンを自分のリポジトリに入れて、新しいゲームを作り始めるまでの手順です。エンジンは `Engine/` フォルダとして入り、ゲームのコードと素材はその外に置きます。

## 完成するフォルダの形

```text
MyGame/                  自分のリポジトリ
├── Engine/              エンジン (直接は直さない。更新は subtree pull で受け取る)
├── Game/                ゲームのコード
├── Assets/              ゲームのモデル・テクスチャ・音・保存したパラメータ
├── main.cpp
├── Game.slnx
├── Game.vcxproj
└── packages.config
```

`Engine/` は必ず `Game.vcxproj` と同じ階層に置きます。エンジンは `Engine/Shaders` や `Engine/Assets` を、この位置からの相対パスで読むためです。

## 手順

1. GitHub で空のリポジトリを作ってクローンし、最初のコミットを 1 つ作る (`README.md` だけでよい)
2. エンジンを `Engine/` に入れる

   ```powershell
   git subtree add --prefix=Engine <エンジンのリポジトリの URL> v1.0.0 --squash
   ```

   `v1.0.0` は使う版のタグ。最新の版はエンジンのリポジトリのタグ一覧で確かめる

3. ひな形をリポジトリの一番上にコピーしてコミットする

   ```powershell
   robocopy Engine\Template . /E
   git add -A
   git commit -m "ひな形からゲームを作成"
   ```

4. `Game.slnx` を Visual Studio で開き、ソリューションを右クリック →「NuGet パッケージの復元」
5. ソリューションエクスプローラーで `Game` を右クリック →「スタートアップ プロジェクトに設定」。`Game` が太字になれば設定できている (最初は DirectXTex が選ばれていて、そのまま起動すると「有効な Win32 アプリケーションではありません」と出る。一度設定すれば次からは覚えている)
6. ビルド構成を **Development** にして F5 で起動する。グリッドだけの画面が出れば完了

ファイル名 (`Game.slnx`・`Game.vcxproj`) やウィンドウのタイトル (`Game/Game.cpp` の `windowTitle`) は、自分のゲームの名前に変えてかまいません。

## ひな形に入っているもの

| ファイル | 内容 |
| :--- | :--- |
| `Game.vcxproj` | `Engine/Engine.vcxitems` (エンジンのソース) と `Engine/Engine.props` (エンジンに必要なビルド設定) を読み込む |
| `main.cpp`・`Game/Game.cpp` | ウィンドウの設定とシーンの登録 |
| `Game/Scene/MainScene` | グリッドを置いただけのシーン |
| `Game/Global/` | シーン名・タグ・入力の割り当て・当たり判定のレイヤー ([コードの決まり](../concepts/conventions.md)) |
| `Assets/Data/FontList.csv` | 文字のフォント。最初はエンジンが持っている NotoSansJP を使う |
| `.gitignore` | `Cache/` やビルド結果を Git に入れないための設定 |
| `README.md` | GitHub でリポジトリを開いたときに出る、ドキュメントへのリンクとビルドの手順。ゲームの説明を書きたいときは、先頭に足す (GitHub が作った README は上書きされる) |

エンジンが使うテクスチャ・モデル・フォントは `Engine/Assets/` に入っているので、ゲーム側で用意しなくても `white1x1` や `noise_39` などの名前で使えます ([アセットの追加](../guides/assets.md))。

## エンジンを更新する

普段の `git pull` ではエンジンは変わりません。新しい版を取り込むときだけ、リポジトリの一番上で次を実行します。

```powershell
.\Engine\UpdateEngine.ps1           # 今の版と、使える版の一覧を出す
.\Engine\UpdateEngine.ps1 v1.1.0    # v1.1.0 に更新する
```

スクリプトは、版が存在するか、コミットしていない変更がないかを確かめてから取り込みます。取り込んだら、表示される手順どおりに `Engine/CHANGELOG.md` を読んでゲームを直し、ビルドして確かめてから push してください。

- 更新の前に、エンジンの担当者が伝えた版の変更点を読む。締め切りの前は取り込まない
- 1 人が取り込んで push すれば、同じリポジトリの他の人は普段の `git pull` で受け取れる
- 取り込んだ後に動かなくなったら、前の版を指定して実行すると戻せる (`.\Engine\UpdateEngine.ps1 v1.0.0` など)。直った版が出たら、同じように指定して取り込む
- 戻すときに `git revert` は使わない。revert で戻した版は、あとで同じ版を指定しても取り込み直せなくなる

:::{note}
「スクリプトの実行が無効」と出るときは、PowerShell で一度だけ `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` を実行してください。
:::

ビルドの設定 (C++ のバージョン・ライブラリ・定義など) は `Engine/Engine.props` にあるので、更新すると自動で反映されます。`Game.vcxproj` を直す必要はありません。

:::{important}
`Engine/` の中は直接直さないでください。直した部分は次の `subtree pull` でぶつかります。エンジンの不具合や欲しい機能は、エンジンの担当者に伝えてください。
:::

## ゲームのプロジェクトの設定を変えるとき

- **変えてよいもの:** 警告レベル・最適化・出力先・ゲームのインクルードディレクトリ・ゲーム独自の定義
- Release は、エディター画面・デバッグ表示・デバッグカメラが入らない設定になっている。Release でも使いたいときは、`Game.vcxproj` の Release の「プリプロセッサの定義」に `ENABLE_IMGUI;ENABLE_DEBUG_DRAW;ENABLE_DEBUG_CAMERA` を足す (エディター画面は F1 で表示する)
- **変えないもの:** `Engine.props` で決めている設定 (C++20・ランタイムライブラリ・`ENABLE_` で始まる定義など)。変えるとエンジンがビルドできなくなるか、動きが変わる
- 構成は Debug・Development・Release の 3 つのまま使う。`Engine.props` が構成の名前で設定を切り替えているため
