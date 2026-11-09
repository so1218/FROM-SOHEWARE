# エフェクトとライト

パーティクル (火花・煙など)、点光源・スポットライト、時間帯と天気の使い方です。

## パーティクルを出す

パーティクルは「プリセット」(見た目の設定) と「エミッター」(発生源) でできています。

- **プリセット**: `Assets/Data/Particle/` の JSON 1 つが 1 種類。「パーティクルエディター」ウィンドウで作り込んで保存する
- **エミッター**: プリセットの名前から作り、位置を決めて `Play()` すると発生する

```cpp
// メンバー。持ち主は ParticleSystem なので、生のポインタで覚えておく
FS::ParticleEmitter* sparkEffect_ = nullptr;

// Initialize で作る
sparkEffect_ = engine_->GetParticleSystem()->SpawnEmitter("shotSparkParticle");

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

:::{warning}
エミッターは**コンストラクタではなく `Initialize()` で作ってください。** シーンを切り替えるとエミッターはすべて消えるため、コンストラクタで作ったものはシーンに入り直したときには残っていません。
また、`Destroy()` したエミッターや、シーンを抜けたあとのエミッターのポインタは使わないでください。
:::

### プリセットを作る

1. `Assets/Data/Particle/` の似たプリセットをコピーして、新しい名前を付ける
2. 起動して「パーティクルエディター」のプリセットで選ぶ
3. 色・大きさ・数・寿命などを調整して「セーブ」を押す

エディターで調整した内容は、セーブする前から画面に反映されます。

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

`Environment` オブジェクト (`Game/GameObject/Field/Environment.h`) をシーンに置くと、空が描かれ、時間帯と天気に合わせて太陽の光・空の色・霧が変わります。

```cpp
objectManager_.Create<Environment>(engine_);
```

時間帯や天気ごとの色は、エディター画面の「環境設定」ウィンドウで調整・保存できます。天気を切り替えると、決めた速さでなめらかに移り変わります。

## 画面全体の効果 (ポストエフェクト)

ブルーム・被写界深度・ビネットなどの画面全体の効果は、シーン名のウィンドウの「ポストエフェクト設定」で調整し、シーンごとに保存します。

## つまずきやすい点

- **パーティクルが出ない。** プリセット名の間違いは「開発用のエラー」ウィンドウに出る。名前が合っているなら、`Play()` を呼んでいるか、位置がカメラに映る場所かを確認する
- **ライトが一瞬だけ光って消える。** 1 回だけ登録している。光らせ続ける間は毎フレーム登録する
