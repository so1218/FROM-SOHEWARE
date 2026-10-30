# FromSoheWare Engine 描画ガイド

本ドキュメントでは、FromSoheWare エンジンにおける各種オブジェクト（3Dモデル、アニメーションモデル、2Dスプライト、ライン）の描画方法と制御について解説します。
エンジンの描画システムは、複雑なレンダリング処理を隠蔽し、最小限の記述で対象を描画できるように設計されています。

---

## 目次
- [1. 3Dモデル (Model) の描画](#1-3dモデル-model-の描画)
- [2. アニメーションモデル (AnimationModel) の描画](#2-アニメーションモデル-animationmodel-の描画)
- [3. 2Dスプライト (Sprite) の描画](#3-2dスプライト-sprite-の描画)
- [4. ライン (Line) の描画](#4-ライン-line-の描画)
- [5. ライトの配置](#5-ライトの配置)
- [6. デバッグ描画 (DebugDraw)](#6-デバッグ描画-debugdraw)

---

## 0. 描画の基本ルール

- 描画クラスは、どれも **コンストラクタで `Engine*` を受け取り**、`std::unique_ptr` でメンバーとして保持します。
- 各クラスの `Draw()` は **描画リクエストの登録** です。実際の描画はフレームの最後 (`Engine::EndFrame()`) にまとめて行われるため、`GameObject::Draw()` の中で呼んでおけば十分です。
- 3D の描画クラスは自分専用の `WorldTransform` を持ち、`Draw()` のたびに行列を更新します。`GameObject` の位置に合わせたい場合は、`Update()` で値をコピーしてください。
- モデル・アニメーション・テクスチャは **名前** で指定します。名前の登録方法は [アセットとデータファイル](assetsGuide.md) を参照してください。

---

## 1. 3Dモデル (Model) の描画

静的な3Dモデルの描画には `FS::Model` クラスを使用します。

### 1-1. モデルの生成
オブジェクトの初期化時（コンストラクタなど）に、エンジンポインタと読み込み済みのモデル名を指定して生成します。

```cpp
#include "Model.h"

// メンバ変数として一意なポインタを持たせる
std::unique_ptr<FS::Model> model_;

// コンストラクタや Initialize() 内で生成 ("bullet" は ModelList.csv に登録した名前)
model_ = std::make_unique<FS::Model>(engine_, "bullet");
```

### 1-2. 位置・回転・拡縮と描画

```cpp
void Bullet::Update()
{
    auto& t = model_->GetTransform();
    t.translation_ = GetTransform().translation_;   // 位置
    t.rotationQuaternion_ = GetTransform().rotationQuaternion_; // 回転 (クォータニオン)
    t.scale_ = { 0.5f, 0.5f, 0.5f };                // 拡縮
}

void Bullet::Draw()
{
    model_->Draw(); // 行列の更新と描画リクエストの登録
}
```

:::{note}
`WorldTransform::UpdateMatrix()` は回転に `rotationQuaternion_` を使います。オイラー角 (`rotation_`) を書き換えただけでは回転しないので、オイラー角で指定したい場合は `Quaternion::QuaternionFromEuler()` で変換してください。
:::

### 1-3. 見た目の変更

| 関数 | 内容 |
| :--- | :--- |
| `SetTexture(name)` | アルベドテクスチャを差し替える（全マテリアル） |
| `SetNormalMapTexture(name)` | 法線マップ |
| `SetColor(const Vector4&)` / `SetColor(uint32_t)` | マテリアルの色 |
| `SetBaseColor(...)` | マテリアルを変更せず、このモデルだけ色を乗算する |
| `SetEmissiveIntensity(float)` | 発光の強さ |
| `SetEnableOutline(bool)` / `SetOutlineWidth(float)` / `SetOutlineColor(...)` | アウトライン |
| `SetEnableDissolve(bool)` / `SetDissolveTexture(name)` | ディゾルブ |
| `SetMaterialColor(index, color)` | マテリアルを 1 つだけ指定して色を変える |

生成直後の `Model` は、同じモデルデータの既定マテリアルを共有しています。上の表のセッターは呼ばれた時点で自動的に `MakeMaterialUnique()` を実行して自分専用のマテリアルに切り替えるため、ほかのインスタンスの見た目には影響しません。逆に、複数のモデルで同じマテリアルを使い回したい場合は `ShareMaterialsFrom()` を使います。

### 1-4. 描画ステート

| 関数 | 値 |
| :--- | :--- |
| `SetBlendMode(BlendMode)` | `kBlendModeNone`, `kBlendModeNormal`, `kBlendModeAdd`, `kBlendModeSubtract`, `kBlendModeMultiply`, `kBlendModeScreen`, `kBlendModeExclusion` |
| `SetCullMode(CullMode)` | `Back` (裏面カリング), `Front`, `None` |
| `SetDepthMode(DepthMode)` | `Write` (通常), `ReadOnly` (半透明・エフェクト向け), `None` (常に最前面) |
| `SetRenderGroup(RenderGroup)` | `Background`, `Opaque`, `AlphaTest`, `Grid`, `Skybox`, `Transparent`, `Particle`, `Trail`, `UI` |

---

## 2. アニメーションモデル (AnimationModel) の描画

スキンメッシュアニメーションには `FS::AnimationModel` を使用します。モデル名と最初に再生するアニメーション名を指定して生成します。

```cpp
#include "AnimationModel.h"

std::unique_ptr<FS::AnimationModel> animationModel_;

// "humanMesh" は ModelList.csv、"humanRun" は AnimationList.csv に登録した名前
animationModel_ = std::make_unique<FS::AnimationModel>(engine_, "humanMesh", "humanRun");
```

`Model` と違い、**`Update()` を毎フレーム呼んでアニメーションを進める** 必要があります。

```cpp
void Player::Update()
{
    animationModel_->Update();
    animationModel_->GetTransform().translation_ = GetTransform().translation_;
    animationModel_->GetTransform().rotationQuaternion_ = GetTransform().rotationQuaternion_;
}

void Player::Draw()
{
    animationModel_->Draw();
}
```

### 2-1. 再生の制御

```cpp
// 名前を指定して再生 (ループ, 再生速度, 前のアニメーションからのブレンド時間[秒])
animationModel_->Play("humanIdle", true, 1.0f, 0.2f);

animationModel_->Stop();            // 一時停止
animationModel_->Resume();          // 再開
animationModel_->ResetAnimation();  // 最初から
animationModel_->SetSpeedScale(1.5f);
animationModel_->SetIsLoop(false);
```

### 2-2. ボーンの手動制御

エイム中に上半身をひねるなど、アニメーション結果に回転を足したい場合は、`Update()` の後に次のように呼びます。

```cpp
animationModel_->Update();
animationModel_->AddJointRotationOffset("mixamorig1:Spine1", offsetRotation); // ボーン名はモデルに依存
animationModel_->PostUpdateSkeleton(); // オフセットを反映してスキニング行列を再計算
```

---

## 3. 2Dスプライト (Sprite) の描画

UI などの 2D 描画には `FS::Sprite` を使用します。

```cpp
#include "Sprite.h"

std::unique_ptr<FS::Sprite> sprite_;

sprite_ = std::make_unique<FS::Sprite>(engine_);
sprite_->SetTexture("titleLogo");          // テクスチャ名 (拡張子なしのファイル名)
sprite_->SetPosition({ 640.0f, 360.0f });   // 画面上の位置 (ピクセル)
sprite_->SetSize({ 512.0f, 256.0f });       // 表示サイズ (ピクセル)
sprite_->SetAnchorPoint({ 0.5f, 0.5f });    // 基準点 (0,0 = 左上, 0.5,0.5 = 中心)
sprite_->SetLayerOrder(10);                 // 重なり順
```

```cpp
void TitleUI::Draw()
{
    sprite_->Draw();
}
```

- 座標系は **左上が (0, 0)、右下が (画面幅, 画面高さ)** のピクセル座標です。
- `SetRotation(float)` の単位はラジアンです。
- `SetIsVisible(false)` で非表示にできます。
- `SetColor(const Vector4&)` で色・不透明度を変えられます。
- 位置やサイズを ImGui で調整して保存したい場合は、`PropertyBinder::BindSprite()` を使うと便利です（[プロパティバインダー ガイド](propertyBinderGuide.md)）。

---

## 4. ライン (Line) の描画

3D 空間に線分を 1 本描くには `FS::Line` を使用します。始点・終点はラインの `WorldTransform` から見たローカル座標です。

```cpp
#include "Line.h"

std::unique_ptr<FS::Line> line_;

line_ = std::make_unique<FS::Line>(engine_);
line_->SetStart({ 0.0f, 0.0f, 0.0f });
line_->SetEnd({ 0.0f, 0.0f, 10.0f });
line_->SetColor(Vector4{ 1.0f, 0.0f, 0.0f, 1.0f });
```

```cpp
void Laser::Update()
{
    line_->GetTransform().translation_ = GetTransform().translation_;
    line_->Update(); // 行列の更新 (Line::Draw は行列を更新しないので先に呼ぶ)
}

void Laser::Draw()
{
    line_->Draw();
}
```

一時的なデバッグ表示が目的なら、後述の `DebugDraw::DrawLine()` の方が手軽です。

---

## 5. ライトの配置

ライトは `LightManager` に登録します。ポイントライトとスポットライトは **毎フレーム登録する** 方式で、フレームの最初に数がリセットされます。

```cpp
void Torch::Update()
{
    engine_->GetLightManager()->SubmitPointLight(
        GetTransform().translation_,     // 位置
        { 1.0f, 0.6f, 0.3f, 1.0f },      // 色
        5.0f,                            // 強さ
        10.0f,                           // 影響範囲
        1.0f);                           // ボリュメトリックフォグへの散乱の強さ
}
```

```cpp
engine_->GetLightManager()->SubmitSpotLight(
    position, color, intensity, distance,
    { 0.0f, -1.0f, 0.0f },  // 向き
    0.866f);                // 照射角 (cos 値。0.866 ≒ 30 度)
```

登録数の上限を超えた場合、`SubmitPointLight()` / `SubmitSpotLight()` は `false` を返します。
平行光源 (太陽光) は `SetDirectionalLight()` で設定します。なお `PlayScene` では、`Ground` が毎フレーム `EnvironmentManager::Update()` を呼び、時間帯・天候に合わせて平行光源のデータを書き換えています。

---

## 6. デバッグ描画 (DebugDraw)

当たり判定の範囲確認などには、static 関数の `FS::DebugDraw` を使います。`ENABLE_DEBUG_DRAW` が定義されていないビルドでは中身が空の関数になるため、`#ifdef` で囲む必要はありません。

```cpp
#include "DebugDraw.h"

FS::DebugDraw::DrawLine(start, end, { 1, 0, 0, 1 });
FS::DebugDraw::DrawSphere(center, 2.0f, { 0, 1, 0, 1 });
FS::DebugDraw::DrawAABB(min, max, { 0, 1, 1, 1 });
FS::DebugDraw::DrawOBB(center, size, rotationMatrix, { 1, 1, 0, 1 });
FS::DebugDraw::DrawFrustum(viewProjection, { 1, 1, 0, 1 });
```
