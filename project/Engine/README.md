# FromSoheWare Engine

DirectX 12 / C++20 のゲームエンジンです。ゲームのリポジトリに `git subtree` で `Engine/` として入れて使います。

## フォルダ

| フォルダ・ファイル | 内容 |
| :--- | :--- |
| `Core/` `Graphics/` `Framework/` など | エンジンのソース |
| `Shaders/` | シェーダー |
| `Assets/` | エンジンが名前で使うテクスチャ・モデル・フォント・描画の設定 |
| `externals/` | 外部ライブラリ |
| `Engine.vcxitems` | エンジンのソースの一覧 (共有プロジェクト) |
| `Engine.props` | エンジンを使うプロジェクトに必要なビルド設定 |
| `Docs/` | ドキュメント (Markdown。Sphinx でビルドすると API リファレンス付きの HTML になる) |
| `Template/` | 新しいゲームのひな形。ビルドの対象ではない |
| `CHANGELOG.md` | 版ごとの変更点 |

## 新しいゲームを始める

```powershell
git subtree add --prefix=Engine <このリポジトリの URL> <版のタグ> --squash
robocopy Engine\Template . /E
```

`Game.slnx` を開き、NuGet パッケージを復元し、`Game` をスタートアップ プロジェクトに設定して、Development でビルドします。詳しくは `Docs/start/new-project.md` を見てください。

## 更新する

```powershell
.\Engine\UpdateEngine.ps1           # 今の版と、使える版の一覧
.\Engine\UpdateEngine.ps1 v1.1.0    # 指定した版に更新する
```

更新の前に `CHANGELOG.md` を読んでください。`Engine/` の中は直接直さず、不具合や要望はエンジンの担当者に伝えてください。
