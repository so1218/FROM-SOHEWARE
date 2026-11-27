# エフェクトとライト

パーティクル (火花・煙など)、点光源・スポットライト、時間帯と天気の使い方です。

## パーティクルを出す

パーティクルは「プリセット」(見た目の設定) と「エミッター」(発生源) でできています。

- **プリセット**: `Assets/Data/Particle/` の JSON 1 つが 1 種類で、ファイル名がプリセットの名前になる。「パーティクルエディター」ウィンドウで調整して保存する
- **エミッター**: プリセットの名前から作り、位置を決めて `Play()` すると発生する

```cpp
// メンバー。持ち主は ParticleSystem なので、生のポインタで覚えておく
FS::ParticleEmitter* sparkEffect_ = nullptr;

// Initialize で作る
sparkEffect_ = engine_->GetParticleSystem()->SpawnEmitter("spark");   // Assets/Data/Particle/spark.json

// 出したいときに
sparkEffect_->SetPosition(hitPoint);
sparkEffect_->Play();
```

`#include "Engine.h"` と `"ParticleSystem.h"` が必要です。

| 関数 | 内容 |
| :--- | :--- |
| `Play()` / `Stop()` | 発生を始める・止める。ループしないプリセットは、決めた時間で自動で止まる |
| `SetPosition(Vector3)` / `SetRotation(Quaternion)` | 発生する位置と向き |
| `SetTargetToFollow(&transform)` | 指定した Transform に付いていく (たき火の煙、キャラクターのオーラなど) |
| `SetFollowOffset(Vector3)` | 付いていくときのずれ |
| `SetEmissionRateMultiplier(float)` | 発生量の倍率。雨の強さを変えるなど |
| `Destroy()` | エミッターを消す。ずっと付いていくエフェクトを持ち主が消えるときに消す |
| `DestroyWhenFinished()` | 今の再生が終わったら自動で消す。倒れた敵の爆発のように、持ち主が先に消える使い切りのエフェクトに使う。出た粒はエミッターが消えても寿命まで残る |

:::{warning}
エミッターは**コンストラクタではなく `Initialize()` で作ってください。** シーンを切り替えるとエミッターはすべて消えるため、コンストラクタで作ったものはシーンに入り直したときには残っていません。
また、`Destroy()` / `DestroyWhenFinished()` したエミッターや、シーンを抜けたあとのエミッターのポインタは使わないでください。
:::

### プリセットを作る

パーティクルエディターには新しいプリセットを作るボタンがないので、JSON ファイルを作ってから調整します。

1. `Assets/Data/Particle/` に `名前.json` を作る。中身は、[チュートリアル 6](../tutorial/06-polish.md#エフェクトのプリセットを作る) の `gemSparkle.json` か、作ってある似たプリセットをコピーする
2. ゲームを起動し直す (プリセットは起動したときに読み込まれる)
3. 「パーティクルエディター」のプリセットで選び、色・大きさ・数・寿命などを調整して「セーブ」を押す

エディターで調整した内容は、セーブする前から画面に反映されます。画像を使うときは、画像を `Assets/Textures/Particles/` に置くと、エディターのテクスチャの選択に出てきます。

## ライトを置く

点光源とスポットライトは**毎フレーム登録します**。フレームの最初に全部消えるので、`Update()` で登録し続ける間だけ光ります。

```cpp
LightManager* lightManager = engine_->GetLightManager();

// 位置, 色, 強さ, 届く距離
lightManager->SubmitPointLight(GetTransform().translation_, { 1.0f, 0.6f, 0.3f, 1.0f }, 5.0f, 10.0f);

// 位置, 色, 強さ, 届く距離, 向き, 照らす広さ (cos。0.866 で約 30 度)
lightManager->SubmitSpotLight(position, { 1.0f, 1.0f, 1.0f, 1.0f }, 8.0f, 20.0f, { 0.0f, -1.0f, 0.0f }, 0.866f);
```

`#include "LightManager.h"` が必要です。ライトの数には上限があり、超えると `false` が返って無視されます。たくさん置くときは、カメラに近いものだけ登録してください。

太陽の光 (平行光源) は、次の「時間帯と天気」が自動で決めます。

## 時間帯と天気

エンジンの `Environment` オブジェクトをシーンに置くと、空が描かれ、時間帯と天気に合わせて太陽の光・空の色・霧が変わります。

```cpp
#include "Environment.h"

objectManager_.Create<Environment>(engine_);
```

**`Environment` を置かないシーンには太陽の光がありません。** モデルを出すシーンには、基本的に置いてください。置かないと、色を付けたモデルも真っ黒に映ります。

時間帯や天気ごとの色は、エディター画面の「環境設定」ウィンドウ (「時間帯」「天候」のタブ) で調整・保存できます。天気を切り替えると、決めた速さでなめらかに移り変わります。

## 画面全体の効果 (ポストエフェクト)

ブルーム・被写界深度・ビネットなどの画面全体の効果は、シーン名のウィンドウの「ポストエフェクト設定」で調整し、シーンごとに保存します。

### シーンの途中で一時的にかける

ボスの登場や必殺技などで一瞬だけ画面演出をかけるときは、`PlayScreenEffect` を呼びます。決めた長さが過ぎると自動で消えるので、戻す処理は要りません。シーンの設定で切っている効果にもかけられます。

```cpp
ScreenEffectRequest request;
request.values.radialBlurStrength = 0.15f;      // 放射ブラー
request.values.chromaOffset = { 0.01f, 0.0f };  // 色収差
request.duration = 0.6f;                        // かけ始めから消えきるまで (秒)
request.fadeOutTime = 0.4f;                     // 最後の 0.4 秒で薄くする
engine_->GetPostEffectManager()->PlayScreenEffect(request);
```

- 値が 0 の効果は重ねません。使える効果は放射ブラー・色収差・画面の縁のビネット・ピクセル化・砂嵐です
- `duration` を 0 以下にすると、`StopScreenEffect` に戻り値を渡すまでかけ続けます (状態が続く間だけかけたいとき)

強弱を繰り返したり震わせたりするときは、揺れの 3 つの値を入れます。揺れは `values` の全部の値に同じようにかかり、最後は揺れながら消えます。

```cpp
request.pulseFrequency = 12.0f;  // 1 秒に 12 回、強弱を繰り返す
request.pulseDepth = 0.7f;       // どれだけ弱まるか (0 なら揺れない、1 なら 0 まで落ちる)
request.pulseRandomness = 0.5f;  // 不規則さ (0 なら規則正しい、上げるほどガタガタ震える)
```

### 値の動きを自分で決める

効果ごとに違う動きにしたいときや、距離・HP などゲームの状態に合わせたいときは、`AddFrameScreenEffect` を毎フレーム呼びます。呼んだフレームだけかかり、呼ばなくなれば消えます。

```cpp
// Update の中で、ボスに近いほど赤い縁を強く震わせる
const float closeness = std::clamp(1.0f - distanceToBoss / 30.0f, 0.0f, 1.0f);
const float shake = 0.5f + 0.5f * std::sin(time_ * 25.0f);

ScreenEffectValues values;
values.vignetteAmount = closeness * (0.4f + 0.3f * shake);
values.vignetteColor = { 0.4f, 0.0f, 0.0f };
engine_->GetPostEffectManager()->AddFrameScreenEffect(values);
```

- 同じ効果が重なったときは強いほうを使います。足し合わせはしません
- ポーズやヒットストップで時間を遅くしている間も進めるかを `useUnscaledTime` で選べます (初期値は進める)
- シーンを作り直すと、重ねている演出は全部消えます
- 「ポストエフェクト設定」の「一時的に重ねる演出の確認」で、見た目をその場で試せます

ずっと値を変えたいときは `GetScreenEffectSettings()` を書き換えます。こちらはシーンに保存される値です。

## つまずきやすい点

- **パーティクルが出ない。** プリセット名の間違いは「開発用のエラー」ウィンドウに出る。名前が合っているなら、`Play()` を呼んでいるか、位置がカメラに映る場所かを確認する。JSON を足したあとは起動し直す
- **モデルが真っ黒に映る。** シーンに `Environment` を置いていない
- **ライトが一瞬だけ光って消える。** 1 回だけ登録している。光らせ続ける間は毎フレーム登録する
