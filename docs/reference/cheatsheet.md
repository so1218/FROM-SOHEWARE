# 早見表

ゲームを作るときによく使う書き方を、やりたいこと別にまとめました。詳しくは各行のガイドを見てください。
コードは `.cpp` に `using namespace FS;` を書いた前提です。

## シーンとオブジェクト

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| シーンにオブジェクトを置く | `objectManager_.Create<Enemy>(engine_, 引数...)` | [GameObject](../guides/gameobject.md) |
| シーンを切り替える | `sceneManager_->RequestSceneChange(SceneName::Play)` | [シーン](../guides/scene.md) |
| ゲームを終える | `engine_->RequestQuit()` | [シーン](../guides/scene.md) |
| 自分の位置 | `GetTransform().translation_` | [GameObject](../guides/gameobject.md) |
| 回転させる | `GetTransform().rotation_ = Quaternion::FromAxisAngle(Vector3::Up(), angle)` | [GameObject](../guides/gameobject.md) |
| 向きたい方向に向ける | `Quaternion::LookRotation(direction, Vector3::Up())` | |
| 一時的に止める | `SetActive(false)` | [GameObject](../guides/gameobject.md) |
| 消す | `Destroy()` | [GameObject](../guides/gameobject.md) |
| タグを付ける・比べる | `SetTag(ObjectTag::Enemy)` / `CompareTag(ObjectTag::Enemy)` | [GameObject](../guides/gameobject.md) |
| 型で探す | `GetManager()->FindObject<Player>()` | [GameObject](../guides/gameobject.md) |
| 更新の順番を決める | `SetUpdatePriority(ObjectUpdateOrder::Player)` | [GameObject](../guides/gameobject.md) |

## 時間と入力

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| 前のフレームからの秒数 | `TimeManager::GetInstance()->GetDeltaTime()` | |
| 時間をゆっくりにする・止める | `TimeManager::GetInstance()->SetTimeScale(0.5f)` | |
| 操作を押した瞬間 | `GameInput::IsTriggered(GameAction::Jump)` | [入力](../guides/input.md) |
| 操作を押している間 | `GameInput::IsPressed(GameAction::Sprint)` | [入力](../guides/input.md) |
| 移動の入力 | `GameInput::GetMoveAxis()` (x が右、y が奥) | [入力](../guides/input.md) |
| パッドを振動させる | `GameInput::StartVibration(左, 右, 秒)` | [入力](../guides/input.md) |

## 見た目

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| モデルを出す | `std::make_unique<Model>(engine_, "名前")` → `Draw()` | [3D モデル](../guides/model.md) |
| 色を変える | `model_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f })` | [3D モデル](../guides/model.md) |
| 一時的に色を掛ける | `model_->SetBaseColor({ 1.0f, 0.5f, 0.5f, 1.0f })` | [3D モデル](../guides/model.md) |
| アニメーションを再生 | `animationModel_->Play("名前", ループ, 速度, つなぐ秒)` | [3D モデル](../guides/model.md) |
| アニメーションが終わったか | `animationModel_->IsFinished()` | [3D モデル](../guides/model.md) |
| 画像を出す | `std::make_unique<Sprite>(engine_)` → `SetTexture("名前")` → `Draw()` | [UI](../guides/ui.md) |
| 文字を出す | `std::make_unique<Text>(engine_)` → `SetText("...")` → `Draw()` | [UI](../guides/ui.md) |
| UI を画面の端に寄せる | `SetAnchor(UIAnchor::TopRight)` | [UI](../guides/ui.md) |
| 明かりを置く (毎フレーム) | `engine_->GetLightManager()->SubmitPointLight(位置, 色, 強さ, 距離)` | [エフェクト](../guides/effects.md) |
| エフェクトを出す | `SpawnEmitter("名前")` (Initialize で) → `SetPosition` → `Play()` | [エフェクト](../guides/effects.md) |
| 画面を揺らす | `cameraManager_->RequestShake(秒, 強さ)` | [カメラ](../guides/camera.md) |

## 音

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| 効果音 | `AudioPlayer::GetInstance().Play("名前", 音量)` | [音](../guides/audio.md) |
| 場所のある音 | `AudioPlayer::GetInstance().Play3D("名前", 位置)` | [音](../guides/audio.md) |
| BGM | `AudioPlayer::GetInstance().PlayBgm("名前", 音量)` | [音](../guides/audio.md) |
| 止める | `AudioPlayer::GetInstance().Stop(ハンドル)` | [音](../guides/audio.md) |

## 当たり判定

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| 球のコライダー | `AddSphereCollider(半径)` | [当たり判定](../guides/collision.md) |
| 箱のコライダー | `AddBoxCollider({ 幅, 高さ, 奥行き })` | [当たり判定](../guides/collision.md) |
| 何と当たるか | `SetCollisionLayer(自分)` / `SetCollisionMask(相手)` | [当たり判定](../guides/collision.md) |
| 当たった瞬間 | `void OnCollisionEnter(Collider* mine, Collider* other) override` | [当たり判定](../guides/collision.md) |
| 当たった相手 | `other->GetOwner()` | [当たり判定](../guides/collision.md) |
| レイを飛ばす | `GetManager()->GetCollisionManager()->Raycast(始点, 向き, 距離, &hit, レイヤー)` | [当たり判定](../guides/collision.md) |

## 調整とデバッグ

| やりたいこと | 書き方 | ガイド |
| :--- | :--- | :--- |
| 値を調整・保存できるようにする | `binder_->Bind("Key", &value_, 既定値)` → `binder_->Draw("Key", "表示名")` | [パラメータ](../guides/parameters.md) |
| 色を調整できるようにする | `binder_->BindColor("Key", &color_, 既定値)` | [パラメータ](../guides/parameters.md) |
| モデルの見た目を調整できるようにする | `binder_->BindModel("Key", model_.get())` → `DrawModel` | [パラメータ](../guides/parameters.md) |
| 直すべき間違いを知らせる | `DevError::Report("内容")` | [デバッグ](../guides/debugging.md) |
| ログを出す | `LOG_INFO("値は {}", value)` | [デバッグ](../guides/debugging.md) |
| 線を描いて確かめる | `DebugDraw::DrawLine(始点, 終点, 色)` | [デバッグ](../guides/debugging.md) |

## 数学

| やりたいこと | 書き方 |
| :--- | :--- |
| 長さ・正規化 | `v.Length()` / `v.Normalize()` |
| 内積・外積 | `a.Dot(b)` / `a.Cross(b)` |
| 成分ごとに掛ける | `Vector3::Scale(a, b)` |
| 2 点の間 | `Math::Lerp(a, b, t)` |
| ベクトルを回転させる | `rotation.RotateVector(v)` |
| 回転のなめらかな補間 | `Quaternion::Slerp(from, to, t)` |
| 度をラジアンに | `Math::ToRadians(degrees)` |
| 乱数 | `Math::RandomFloat(min, max)` |
| イージング | `Easing::Evaluate(EasingType::EaseOutCubic, t)` |

## 決まった場所に書くもの

| もの | 場所 |
| :--- | :--- |
| シーン名・タグ・更新の順番 | `Game/Global/GameDefine.h` |
| 操作と割り当て | `Game/Global/GameInput.h` / `GameInput.cpp` |
| 当たり判定のレイヤー | `Game/Global/CollisionConfig.h` |
| シーンの登録 | `Game/Game.cpp` |
| 保存した値 | `Assets/Data/GlobalVariables/*.json` |
