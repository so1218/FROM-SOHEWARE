# 3D モデルとアニメーション

動かないモデルは `FS::Model`、ボーンで動くモデルは `FS::AnimationModel` で表示します。
どちらも GameObject のメンバーとして持ち、`Draw()` の中で `Draw()` を呼ぶだけで表示されます。

## モデルを出す

```cpp
// メンバー
std::unique_ptr<FS::Model> model_;

// コンストラクタで。名前は Assets/Models にあるファイル名 (拡張子なし)
model_ = std::make_unique<Model>(engine_, "tree");

// Update で、オブジェクトの位置に合わせる
model_->GetTransform().translation_ = GetTransform().translation_;
model_->GetTransform().rotation_ = GetTransform().rotation_;

// Draw で
model_->Draw();
```

モデルは自分の位置 (`model_->GetTransform()`) を持っていて、GameObject の位置とは別です。毎フレーム合わせるのを忘れると、原点に出たままになります。

:::{tip}
`Draw()` は「描く予定を登録する」だけで、実際の描画はフレームの最後にまとめて行われます。`Draw()` を呼ぶ順番で前後関係は変わりません。
:::

## 見た目を変える

| 関数 | 内容 |
| :--- | :--- |
| `SetColor(Vector4)` | 色。テクスチャに掛け合わされる |
| `SetTexture(name)` | 色のテクスチャを差し替える |
| `SetNormalMapTexture(name)` | 法線マップ (凹凸) |
| `SetEmissiveIntensity(float)` | 発光の強さ |
| `SetBaseColor(Vector4)` | マテリアルを変えずに、このモデルだけ色を掛ける。点滅や被弾時の赤みなど、一時的な変化に使う |
| `SetEnableOutline(bool)` / `SetOutlineColor` / `SetOutlineWidth` | 輪郭線 |
| `SetEnableDissolve(bool)` / `SetDissolveTexture(name)` | 溶けて消える表現 |

モデルファイルの中で指定したテクスチャは使われず、最初は白で表示されます。テクスチャは `SetTexture` か、下のエディター画面での調整で付けてください。

同じモデルを読み込んだ複数のインスタンスは、最初は見た目 (マテリアル) を共有しています。上の関数を呼ぶと自動で自分専用になるので、ほかのインスタンスには影響しません。

### エディター画面で調整する

色・粗さ・金属感などは、コードで決めるより [PropertyBinder](parameters.md) に登録して画面を見ながら決める方が早いです。

```cpp
binder_->BindModel("Body", model_.get());   // Initialize で
binder_->DrawModel("Body", "体のモデル");    // DebugGui で
```

位置・回転・大きさも一緒に登録されます。`Update()` で毎フレーム位置を上書きしているモデルでは、画面の位置の欄を動かしても戻ります。

## 半透明・描画の種類

| 関数 | よく使う値 |
| :--- | :--- |
| `SetRenderGroup(RenderGroup)` | `Opaque` (不透明。既定)、`Transparent` (半透明) |
| `SetBlendMode(BlendMode)` | `kBlendModeNormal` (半透明)、`kBlendModeAdd` (加算。光るエフェクト) |
| `SetDepthMode(DepthMode)` | `ReadOnly` (半透明やエフェクト。後ろのものを隠さない) |
| `SetCullMode(CullMode)` | `None` (裏面も描く。葉や布) |

半透明にするときは `RenderGroup::Transparent` と `kBlendModeNormal` をセットで指定します。

## アニメーションさせる

```cpp
// コンストラクタで。モデル名と、最初に再生するアニメーション名
animationModel_ = std::make_unique<AnimationModel>(engine_, "humanMesh", "humanIdle");

// Update で。アニメーションを進めるので、毎フレーム必ず呼ぶ
animationModel_->Update();
animationModel_->GetTransform().translation_ = GetTransform().translation_;

// Draw で
animationModel_->Draw();
```

アニメーションの切り替えは `Play` です。前のアニメーションから、指定した秒数をかけてなめらかにつながります。

```cpp
// 名前, ループするか, 再生速度, つなぐ時間 (秒)
animationModel_->Play("humanRun", true, 1.0f, 0.2f);

// 1 回だけ再生して、終わったら待機に戻す
animationModel_->Play("humanRoll", false);
if (animationModel_->IsFinished())
{
    animationModel_->Play("humanIdle");
}
```

| 関数 | 内容 |
| :--- | :--- |
| `IsFinished()` | ループしないアニメーションが最後まで再生されたか |
| `GetNormalizedTime()` | 再生位置 (0 ~ 1)。「半分まで振ったら当たり判定を出す」などに使う |
| `SetSpeedScale(float)` | 再生速度 |
| `Stop()` / `Resume()` | 一時停止・再開 |
| `GetJointWorldMatrix(name)` | ボーンのワールド行列。手に武器を持たせるときに使う |

### アニメーションのファイル

アニメーションは、メッシュを含まない glTF ファイルとして `Assets/Models` に置きます。ファイル名 (拡張子なし) がアニメーション名になります。
Mixamo などからダウンロードしたものは、アニメーションごとに 1 ファイルに分けて書き出してください。

## つまずきやすい点

- **`AnimationModel::Update()` を呼び忘れると動かない。** `Model` と違い、毎フレームの `Update()` が必要
- **モデル名を間違えると、「開発用のエラー」ウィンドウに名前と場所が出る。** 名前は拡張子なしのファイル名。フォルダ名は含めない
- **同じ名前のファイルが 2 つあると、片方しか読まれない。** フォルダが違っても名前は重ならないようにする
