# UI (画像と文字)

画面に画像を出すには `FS::Sprite`、文字を出すには `FS::Text` を使います。
UI 用の GameObject を作り、その中で持つのが基本です (例: `Game/GameObject/UI/GameUI.cpp`)。

## 画面の座標

- UI の座標は、**画面の高さを 720 とした単位**です。解像度が変わっても同じ見た目になるよう、自動で拡大・縮小されます
- 位置は「画面のどこを基準にするか (アンカー)」からのずれで指定します。右上の UI は右上を基準にすると、画面の横幅が変わっても崩れません

```cpp
sprite->SetAnchor(UIAnchor::TopRight);   // 画面の右上を基準にし、画像の右上を合わせる
sprite->SetPosition({ -20.0f, 20.0f });   // そこから左に 20、下に 20
```

アンカーは `TopLeft` `TopCenter` `TopRight` `MiddleLeft` `Center` `MiddleRight` `BottomLeft` `BottomCenter` `BottomRight` の 9 つです。

## 画像を出す

```cpp
// コンストラクタで
icon_ = std::make_unique<Sprite>(engine_);
icon_->SetTexture("heart");               // Assets/Textures にあるファイル名 (拡張子なし)
icon_->SetAnchor(UIAnchor::TopLeft);
icon_->SetPosition({ 20.0f, 20.0f });
icon_->SetSize({ 64.0f, 64.0f });

// Draw で
icon_->Draw();
```

| 関数 | 内容 |
| :--- | :--- |
| `SetColor(Vector4)` | 色と不透明度。`{ 1, 1, 1, 0.5f }` で半透明 |
| `SetRotation(float)` | 回転 (ラジアン) |
| `SetLayerOrder(int)` | 重なり順。大きいほど手前 |
| `SetIsVisible(bool)` | 表示・非表示 |
| `SetUVRect({ x, y, 幅, 高さ })` | テクスチャの一部だけを出す (0 ~ 1 で指定) |

### ゲージを作る

HP ゲージのように伸び縮みするものは、背景と中身の 2 枚を重ね、中身の幅を変えます。

```cpp
const float ratio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
gaugeFill_->SetSize({ gaugeSize_.x * ratio, gaugeSize_.y });
```

左端を基準に縮めたいので、アンカーは左側 (`TopLeft` など) にしておきます。実例は `Game/GameObject/Character/Player/PlayerHealth.cpp` です。

## 文字を出す

```cpp
// コンストラクタで。フォント名を省略すると FontList.csv の先頭のフォント
scoreText_ = std::make_unique<Text>(engine_);
scoreText_->SetAnchor(UIAnchor::TopRight);
scoreText_->SetPosition({ -20.0f, 20.0f });
scoreText_->SetFontSize(32.0f);

// Update で
scoreText_->SetText(std::format("SCORE {}", score_));

// Draw で
scoreText_->Draw();
```

| 関数 | 内容 |
| :--- | :--- |
| `SetFont(name)` | フォントを変える。名前は `Assets/Data/FontList.csv` の Name 列 |
| `SetColor(Vector4)` | 文字の色 |
| `SetOutline(太さ, 色)` | 縁取り |
| `SetShadow(ずらす量, 色)` | 影 |
| `SetAlign(TextAlign)` | 複数行のときの揃え方 |
| `SetWrapLength(float)` | この長さを超えたら折り返す |
| `SetOpacity(float)` | 縁取りも含めた不透明度。フェードイン・アウトに使う |
| `MeasureSize()` | 文字列の大きさ。背景の帯を文字に合わせるときに使う |

## エディター画面で配置する

UI の位置や大きさは、コードで決めるより画面を見ながら決める方が早いです。[PropertyBinder](parameters.md) に登録すると、位置・大きさ・画像・文字・色をまとめて調整・保存できます。

```cpp
// Initialize で
binder_->BindSprite("HpGauge", hpGauge_.get());
binder_->BindTextElement("Title", titleText_.get());
// スコアのように、コードで毎フレーム書き換える文字は、文字の内容を登録しない
binder_->BindTextElement("Score", scoreText_.get(), false);

// DebugGui で
binder_->DrawSprite("HpGauge", "HP ゲージ");
binder_->DrawTextElement("Title", "タイトル");
binder_->DrawTextElement("Score", "スコア");
```

:::{important}
`BindSprite` / `BindTextElement` で登録したものは、**位置や大きさを画面で決めて保存する**前提です。
まだ保存していない項目は既定値 (位置 0、大きさ 100 など) になり、登録より前にコードで設定した値は使われません。登録したら、最初に一度画面で配置して保存してください。
:::

## フォントを足す

1. フォントファイル (`.ttf`) を `Assets/Data/Fonts/` に置く
2. `Assets/Data/FontList.csv` に 1 行足す

```text
Name,Path,PixelHeight,Charset
Main,NotoSansJP-Bold.ttf,48
Pop,MyPopFont.ttf,72
```

| 列 | 内容 |
| :--- | :--- |
| Name | コードで使う名前 |
| Path | `Assets/Data/Fonts/` からのファイルの場所 |
| PixelHeight | 文字を作るときの大きさ。大きく表示する文字が多いなら大きくする |
| Charset | 使う漢字の一覧ファイル。省略すると `Charset.txt` |

英数字・ひらがな・カタカナ・記号はいつでも使えます。漢字は Charset のファイルに書いてあるものだけが使えるので、出ない漢字があればそのファイルに足してください。

## つまずきやすい点

- **漢字が出ない。** Charset のファイルにその漢字がない
- **右や下に寄せた UI が、解像度を変えるとずれる。** 左上を基準にして大きな数値で置いている。寄せたい側のアンカーを使う
- **UI の重なり順が思った通りにならない。** `Draw()` を呼ぶ順番ではなく `SetLayerOrder()` で決まる
