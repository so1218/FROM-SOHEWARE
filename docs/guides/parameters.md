# パラメータの調整と保存

`FS::PropertyBinder` に変数を登録すると、エディター画面で値を変えられるようになり、JSON への保存と起動時の読み込みも自動で行われます。
「速さ」「色」「UI の位置」のように、遊びながら決めたい値はすべてこれで管理します。

## 基本の使い方

使い方は「作る → 登録する → 表示する」の 3 つです。

```cpp
// 1. 作る (コンストラクタで)。"Player" が保存先の名前になる
binder_ = std::make_unique<PropertyBinder>(engine_, "Player");

// 2. 登録する (Initialize で)。保存した値があれば読み込み、なければ既定値を使う
binder_->Bind("MoveSpeed", &moveSpeed_, 6.0f);

// 3. 表示する (DebugGui で)
void Player::DebugGui()
{
    ImGui::Begin("プレイヤー");
    binder_->Draw("MoveSpeed", "移動の速さ");
    ImGui::End();
}
```

値はエディター画面で変えた瞬間に反映されます。保存は「パラメータの保存」ウィンドウの「すべての変更を保存」で行い、`Assets/Data/GlobalVariables/<保存先の名前>.json` に書かれます。

## 登録できるもの

| 登録の関数 | 対象 | 画面での見た目 |
| :--- | :--- | :--- |
| `Bind(key, &value, 既定値, 変化量, 最小, 最大)` | `int` `float` `bool` `Vector2` `Vector3` `Vector4` | ドラッグで数値を変える欄 (bool はチェックボックス) |
| `BindColor(key, &color, 既定値)` | `Vector3` `Vector4` `uint32_t` | 色の選択 |
| `BindRotation(key, &quaternion)` | `Quaternion` | X・Y・Z の回転角 (ラジアン) で編集・保存する。保存した値がなければ今の回転のまま |
| `BindCombo(key, &index, 既定値, "A\0B\0C\0")` | `int` | 選択肢から選ぶ |
| `BindString(key, 既定値, 変わったときの処理)` | 文字列 | 入力欄 |
| `BindTexture(key, 初期値, 変わったときの処理)` | テクスチャ名 | 読み込み済みのテクスチャから選ぶ |
| `BindModel(group, model)` | `Model` | 位置・色・テクスチャ・マテリアルをまとめて (`DrawModel` で表示) |
| `BindAnimationModel(group, model)` | `AnimationModel` | 同上 (`DrawAnimationModel` で表示) |
| `BindSprite(group, sprite)` | `Sprite` | 位置・大きさ・画像・色をまとめて (`DrawSprite` で表示) |
| `BindTextElement(group, text)` | `Text` | 文字・フォント・位置・色をまとめて (`DrawTextElement` で表示) |

変化量・最小・最大は省略できます。最小と最大を両方 0 にすると、範囲の制限なしになります。

```cpp
binder_->Bind("JumpPower", &jumpPower_, 10.0f, 0.1f, 0.0f, 50.0f);
binder_->BindColor("LightColor", &lightColor_, { 1.0f, 0.6f, 0.3f, 1.0f });
binder_->BindModel("Body", model_.get());
binder_->BindSprite("HpGauge", hpGauge_.get());

// 表示は登録したキーで
binder_->Draw("JumpPower", "ジャンプの強さ");
binder_->DrawModel("Body", "体のモデル");
binder_->DrawSprite("HpGauge", "HP ゲージ");
```

登録した順にすべて出したいときは `binder_->DrawAll()` も使えます。

## 値が変わったときに処理をする

`float` と `Vector3` の `Bind` には、値が変わったときに呼ぶ関数を渡せます。行列の作り直しなど、値を変えただけでは反映されないものに使います。

```cpp
binder_->Bind("Fov", &fov_, 0.45f, 0.01f, 0.1f, 3.0f, [this]() { RebuildProjection(); });
```

## 保存先を階層にする

保存先の名前を複数渡すと、1 つの JSON の中で階層に分けて保存されます。同じ種類のものがたくさんあるときに使います。

```cpp
// EnvironmentProps.json の中の "Prop_3" に保存される
binder_ = std::make_unique<PropertyBinder>(engine_, "EnvironmentProps", "Prop_3");
```

## シーンごとの設定

各シーンは、シーン名で保存する `binder_` を最初から持っています。カメラ・ライト・ポストエフェクトの設定はここに保存され、シーン名のウィンドウで調整できます。
シーンだけで使う値も、ここに登録できます。

```cpp
void PlayScene::OnInitialize()
{
    binder_->Bind("TimeLimit", &timeLimit_, 300.0f, 1.0f, 1.0f, 3600.0f);
}
```

## つまずきやすい点

- **キー名を変えると、保存した値が読まれなくなる。** 名前を変えるときは、`Assets/Data/GlobalVariables/` の JSON のキーも一緒に書き換える
- **`Draw` するキーは `Bind` 済みでなければならない。** 綴りを間違えると「開発用のエラー」ウィンドウに、間違えたキーと場所が出る
- **保存しないと消える。** 調整した値は「すべての変更を保存」を押すまでファイルに書かれない
- **単位を書いておく。** `Draw` の 2 つ目の引数に「(秒)」「(ラジアン)」などを入れておくと、調整する人が迷わない
