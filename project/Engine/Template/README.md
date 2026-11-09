## ドキュメント

エンジンの使い方は `Engine/Docs/` にあります。GitHub 上でもそのまま読めます。

| 目的 | 読むところ |
| :--- | :--- |
| 目次 | [ドキュメントのトップ](Engine/Docs/index.md) |
| まず動かしたい | [はじめに](Engine/Docs/start/index.md) |
| ゲームの作り方を一通り知りたい | [チュートリアル](Engine/Docs/tutorial/index.md) |
| やりたいことのやり方を知りたい | [機能別ガイド](Engine/Docs/guides/index.md) |
| 関数を調べたい | [早見表](Engine/Docs/reference/cheatsheet.md) |
| 完成版を作りたい | [完成版を作る](Engine/Docs/guides/packaging.md) |
| エンジンの変更点 | [CHANGELOG](Engine/CHANGELOG.md) |

## ビルド

1. `Game.slnx` を Visual Studio で開く
2. ソリューションを右クリック →「NuGet パッケージの復元」(初回だけ)
3. `Game` をスタートアップ プロジェクトに設定する (初回だけ)
4. Development でビルドして起動する

## フォルダ

| フォルダ | 内容 |
| :--- | :--- |
| `Game/` | ゲームのコード |
| `Assets/` | ゲームのモデル・テクスチャ・音・保存したパラメータ |
| `Engine/` | エンジン。直接は直さない |

## エンジンの更新

```powershell
.\Engine\UpdateEngine.ps1           # 今の版と、使える版の一覧
.\Engine\UpdateEngine.ps1 v1.1.0    # 指定した版に切り替える
```
