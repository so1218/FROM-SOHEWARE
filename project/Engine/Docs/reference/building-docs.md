# このドキュメントのビルド方法

このドキュメントは [Sphinx](https://www.sphinx-doc.org/) で作っています。文章は Markdown (MyST)、API リファレンスは [Doxygen](https://www.doxygen.nl/) が C++ のヘッダーから作った XML を [Breathe](https://breathe.readthedocs.io/) で取り込んでいます。

## 必要なツール (初回だけ)

```powershell
winget install Python.Python.3.12
winget install DimitriVanHeesch.Doxygen
pip install -r Engine/Docs/requirements.txt
```

インストールしたら、新しいターミナルで `doxygen --version` が動くことを確認してください。
PATH が通っていないときは、環境変数 `DOXYGEN` に `doxygen.exe` のフルパスを設定すると、それが使われます。

## ビルド

パスはゲームのリポジトリでの位置です。エンジンの担当者のリポジトリでは `project/Engine/Docs` になります。

```powershell
cd Engine\Docs
.\make.bat html
```

`Engine/Docs/_build/html/index.html` をブラウザで開くと確認できます。ビルドのたびに Doxygen も動くので、ヘッダーを変えたときも同じコマンドで API リファレンスが更新されます。

| コマンド | 内容 |
| :--- | :--- |
| `.\make.bat html` | HTML を作る |
| `.\make.bat clean` | ビルド結果 (`_build/`) を消す |
| `.\make.bat live` | 確認用のサーバーを起動し、保存するたびに作り直して再読み込みする |

文章だけを直しているときは、Doxygen を省略して速くできます (2 回目から)。

```powershell
$env:SKIP_DOXYGEN = "1"
.\make.bat html
```

## ページの構成

ページは、読む目的ごとにフォルダを分けています ([Diátaxis](https://diataxis.fr/) という考え方)。新しいページは、目的に合うフォルダに置いてください。

| フォルダ | 種類 | 書き方 |
| :--- | :--- | :--- |
| `start/` | はじめに | 起動するまでと、エディター画面の使い方 |
| `tutorial/` | チュートリアル | 手を動かして覚える。説明は最小限にして、各ステップの終わりで動きを確かめられるようにする |
| `guides/` | 機能別ガイド | 1 ページ 1 テーマ。「基本の使い方 → よく使う設定 → つまずきやすい点」の順。全関数の説明は書かない |
| `concepts/` | エンジンの仕組み | なぜそうなっているかの説明。手順は書かない |
| `reference/` | リファレンス | 早見表と API。説明や手順は書かない |

- ページを足したら、そのフォルダの `index.md` の `toctree` にファイル名 (拡張子なし) を足す。足さないとサイドバーに出ず、ビルドで警告が出る
- 図は Mermaid で書く。` ```mermaid ` のコードブロックは、Sphinx でも GitHub でも図になる
- 注意書きは `:::{note}` ～ `:::` で囲む (`note` / `tip` / `important` / `warning`)
- コードの例は、実際にビルドが通るものを書く。関数名や引数を変えたら、ドキュメントの例も直す

## API リファレンスにクラスを足す

`reference/api.md` に、ゲームから使うクラスだけを載せています。載せたいクラスができたら、同じ形で足してください。

````text
(api-fs-newclass)=
### FS::NewClass

ヘッダー: `Engine/Core/NewClass.h`

```{doxygenclass} FS::NewClass
:members:
:undoc-members:
```
````

## API リファレンスに説明文を載せるには

Doxygen は `//` のコメントを読まないため、今の API リファレンスには宣言だけが表示されます。説明を載せたいところは、ヘッダーのコメントを `///` で書くと次のビルドから表示されます。

```cpp
/// 1 回だけ鳴らす。volume は 0 ~ 1
int Play(const std::string& name, float volume = 1.0f);

float moveSpeed_ = 6.0f;   ///< 宣言の後ろに書くときは ///<
```

## GitHub Pages での公開

エンジンの担当者のリポジトリでは、`.github/workflows/Docs.yml` が `master` ブランチへの push のたびにドキュメントを作って GitHub Pages に公開します。ドキュメントかヘッダーが変わったときだけ動きます。

初回だけ、GitHub のリポジトリの **Settings** → **Pages** → **Build and deployment** の **Source** を **GitHub Actions** にしてください。

:::{warning}
非公開 (private) のリポジトリで GitHub Pages を使うには有料プランが必要です。また、公開するとコードの構造が誰でも見られるようになるので、公開してよいか確認してから有効にしてください。
:::
