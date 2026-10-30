# このドキュメントのビルド方法

このドキュメントは [Sphinx](https://www.sphinx-doc.org/) で作られています。ガイドの文章は Markdown (MyST)、API リファレンスは [Doxygen](https://www.doxygen.nl/) が C++ ヘッダーから生成した XML を [Breathe](https://breathe.readthedocs.io/) で取り込んで作っています。

---

## 1. 必要なツール (初回のみ)

### 1-1. Python

Python 3.10 以降をインストールします。

```powershell
winget install Python.Python.3.12
```

### 1-2. Doxygen

```powershell
winget install DimitriVanHeesch.Doxygen
```

公式サイト (<https://www.doxygen.nl/download.html>) のインストーラーでも構いません。インストール後、新しいターミナルで `doxygen --version` が動くことを確認してください。
PATH が通っていない場合は、環境変数 `DOXYGEN` に `doxygen.exe` のフルパスを設定すると、それが使われます。

### 1-3. Python パッケージ

リポジトリのルートで次を実行します。

```powershell
pip install -r docs/requirements.txt
```

---

## 2. ビルド

```powershell
cd docs
.\make.bat html
```

ビルドが終わったら `docs/_build/html/index.html` をブラウザで開きます。
ビルドのたびに Doxygen が自動で実行されるため、ヘッダーを変更した場合も同じコマンドで API リファレンスが更新されます。

| コマンド | 内容 |
| :--- | :--- |
| `.\make.bat html` | HTML をビルドする |
| `.\make.bat clean` | ビルド結果 (`_build/`) を削除する |
| `.\make.bat live` | プレビュー用サーバーを起動し、ファイルを保存するたびに自動で再ビルド・再読み込みする |

ガイドの文章だけを編集しているときは、Doxygen の実行を省略してビルドを速くできます (2 回目以降のみ)。

```powershell
$env:SKIP_DOXYGEN = "1"
.\make.bat html
```

---

## 3. ページの追加と編集

- ガイドは `docs/` 直下の `.md` ファイルです。GitHub 上でもそのまま読めます。
- 新しいページを追加したら、`docs/index.md` の末尾にある `toctree` にファイル名 (拡張子なし) を追加してください。追加しないとサイドバーに表示されず、ビルド時に警告が出ます。
- 図は Mermaid で書けます。` ```mermaid ` のコードブロックは、Sphinx でも GitHub でも図として表示されます。
- 注意書きには次の記法が使えます (`note` / `warning` / `important` / `tip` など)。GitHub 上では記号がそのまま表示されます。

  ```text
  :::{note}
  ここに注意書きを書く
  :::
  ```

- API リファレンスの各ページ (`docs/api/*.md`) は、ヘッダーのクラス一覧をもとに作成しています。新しいクラスを追加した場合は、該当するページに次のように追記してください。

  ````text
  ## FS::NewClass

  ヘッダー: `Engine/Core/NewClass.h`

  ```{doxygenclass} FS::NewClass
  :members:
  :protected-members:
  :undoc-members:
  ```
  ````

---

## 4. API リファレンスに説明文を載せるには

Doxygen は `//` のコメントを読み取らないため、現在の API リファレンスにはクラス名と関数の宣言だけが表示されます。説明文を表示したい箇所は、ヘッダーのコメントを `///` で書くと、次回のビルドから反映されます。

```cpp
/// @brief ゲームの設定値やパラメータをグループごとに管理するクラス
class GlobalVariables
{
public:
    /// @brief 値をセットする
    /// @param groupPath グループの階層 (例: {"Player", "Weapon"})
    /// @param key       項目名
    /// @param value     設定する値
    template <typename T>
    void SetValue(const std::vector<std::string>& groupPath, const std::string& key, const T& value);

    int dragSensitivityInt_ = 1; ///< 宣言の後ろに書く場合は ///< を使う
};
```

`///` の 1 行目は自動で概要 (brief) として扱われるため、`@brief` は省略しても構いません。

---

## 5. GitHub Pages での公開

`.github/workflows/Docs.yml` に、`master` ブランチへの push 時にドキュメントをビルドして GitHub Pages に公開するワークフローを用意しています。`docs/` 以下か、エンジン・ゲームのヘッダーが変更されたときだけ実行されます。

初回のみ、GitHub のリポジトリで次の設定を行ってください。

1. **Settings** → **Pages** を開く
2. **Build and deployment** の **Source** を **GitHub Actions** に変更する

以後、push すると Actions タブの「Docs」ワークフローが実行され、完了後に `https://<ユーザー名>.github.io/<リポジトリ名>/` で公開されます。

:::{warning}
リポジトリが非公開 (private) の場合、GitHub Pages を使うには有料プランが必要です。またページを公開するとソースコードの構造が誰でも見られるようになるため、公開してよいかを確認してから有効にしてください。
:::
