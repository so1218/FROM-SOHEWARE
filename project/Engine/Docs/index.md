# FromSoheWare Engine ドキュメント

FromSoheWare は、C++20 と DirectX 12 で作られたゲームエンジンです。
このドキュメントだけを読めば、エンジンを初めて触る人でも 1 本のゲームを作り切れることを目指しています。

![FromSoheWare Engine のデモシーン](textures/scene_01.png)

## 読む順番

目的に合わせて、次の 4 つから選んでください。

| 目的 | 読むところ | 内容 |
| :--- | :--- | :--- |
| まず動かしたい | [はじめに](start/index.md) | ビルドして起動し、エディター画面の使い方を覚える |
| ゲームの作り方を一通り知りたい | [チュートリアル](tutorial/index.md) | ボールを転がしてジェムを集める小さなゲームを、最初から最後まで作る |
| やりたいことのやり方を知りたい | [機能別ガイド](guides/index.md) | 「モデルを出す」「当たり判定を付ける」など、1 ページ 1 テーマの手順書 |
| 仕組みを知りたい・エンジンを改造したい | [エンジンの仕組み](concepts/index.md) | 1 フレームの流れ、描画パイプライン、コードの決まり |

関数名や引数を調べたいときは [早見表](reference/cheatsheet.md) と [API リファレンス](reference/api.md) を使ってください。

:::{tip}
初めての人は、[はじめに](start/index.md) → [チュートリアル](tutorial/index.md) の順に読むのがおすすめです。
チュートリアルを終えたら、作りたいゲームに必要なガイドだけを拾い読みすれば十分です。
:::

## このドキュメントの構成について

ページは「何のために読むか」で 4 種類に分けています ([Diátaxis](https://diataxis.fr/) という考え方です)。

- **チュートリアル**: 手を動かして覚える。説明は最小限にして、完成まで迷わず進めることを優先する
- **機能別ガイド**: やりたいことが決まっている人向けの手順書。よく使う設定と、つまずきやすい点だけを書く
- **エンジンの仕組み**: なぜそうなっているかの説明。読まなくてもゲームは作れる
- **リファレンス**: 関数や設定の一覧。調べるときに使う

```{toctree}
:hidden:
:caption: はじめに
:maxdepth: 2

start/index
start/new-project
```

```{toctree}
:hidden:
:caption: チュートリアル
:maxdepth: 2

tutorial/index
```

```{toctree}
:hidden:
:caption: 機能別ガイド
:maxdepth: 2

guides/index
```

```{toctree}
:hidden:
:caption: エンジンの仕組み
:maxdepth: 2

concepts/index
```

```{toctree}
:hidden:
:caption: リファレンス
:maxdepth: 2

reference/index
```
