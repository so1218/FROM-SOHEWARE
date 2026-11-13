# 6. 音とエフェクトを付ける

ジェムを拾ったときに、効果音を鳴らして光のエフェクトを出します。

## 効果音のファイルを置く

好きな効果音を `gemGet.mp3` という名前で `Assets/Audio/` に置きます (`.wav` でも構いません。`Assets/Audio/SE/` のようにフォルダを作って分けても大丈夫です)。
`Assets/` の中に置いたファイルは起動したときに自動で読み込まれ、ファイル名 (拡張子なし) で使えるようになります。

## エフェクトのプリセットを作る

エフェクト (パーティクル) は、`Assets/Data/Particle/` にある **プリセット** (見た目の設定を書いた JSON) の名前で出します。
ひな形から始めたゲームにはプリセットが 1 つもないので、まず作ります。

1. `Assets/Data/Particle/` に `gemSparkle.json` というファイルを作り、下の内容を貼り付けて保存する
2. ゲームを起動し直す (プリセットは起動したときに読み込まれる)

```json
{
    "EmitterConfig": {
        "amount": 12,
        "spawnInterval": 0.05,
        "duration": 0.1,
        "lifetime": 0.6,
        "looping": false,
        "playOnAwake": false
    },
    "ParticleConfig": {
        "intensity": 4.0,
        "velocityModule": {
            "enabled": true,
            "speed": 4.0,
            "randomDirection": true,
            "angleRange": 180.0,
            "direction": [0.0, 1.0, 0.0]
        },
        "physicsModule": {
            "enabled": true,
            "gravity": [0.0, -6.0, 0.0]
        },
        "sizeOverLifetimeModule": {
            "enabled": true,
            "startScale": [0.3, 0.3, 0.3],
            "endScale": [0.0, 0.0, 0.0]
        },
        "colorOverLifetimeModule": {
            "enabled": true,
            "startColor": 1724776447,
            "endColor": 1724776192
        }
    }
}
```

水色の小さな四角が、上に飛び散って落ちながら小さくなる設定です。書いていない項目は既定値になります。
見た目は、あとでエディター画面で調整できます。

## 鳴らして、エフェクトを出す

エフェクトは、プリセットの名前から **エミッター** (発生源) を作り、好きなときに `Play()` します。

`Ball.h` に、エミッターを覚えておく変数を足します。

```cpp
#include "ParticleEmitter.h"   // 追加

private:
    // ...
    FS::ParticleEmitter* pickupEffect_ = nullptr;   // 追加。持ち主は ParticleSystem なので、delete しない
```

`Ball.cpp` を次のように変えます。

```cpp
#include "Engine.h"           // 追加
#include "AudioPlayer.h"      // 追加
#include "ParticleSystem.h"   // 追加

void Ball::Initialize()
{
    GetTransform().translation_ = { 0.0f, 1.0f, 0.0f };
    score_ = 0;
    binder_->Bind("MoveSpeed", &moveSpeed_, 6.0f, 0.1f, 0.0f, 50.0f);

    // エミッターはシーンを切り替えると消えるので、シーンに入るたびに作る
    pickupEffect_ = engine_->GetParticleSystem()->SpawnEmitter("gemSparkle");
}

void Ball::OnCollisionEnter(Collider* mine, Collider* other)
{
    if (other->GetOwner()->CompareTag(ObjectTag::Item))
    {
        ++score_;

        // 音量は 0 ~ 1
        AudioPlayer::GetInstance().Play("gemGet", 0.5f);

        // 拾ったジェムの位置でエフェクトを出す
        pickupEffect_->SetPosition(other->GetOwner()->GetTransform().translation_);
        pickupEffect_->Play();
    }
}
```

:::{warning}
エミッターは**コンストラクタではなく `Initialize()` で作ってください。**
シーンを切り替えるとエミッターはすべて消えるため、コンストラクタで作ったものはシーンに入り直したときに使えなくなっています。
:::

## 動かしてみる

ジェムを拾うと、音が鳴って光が飛び散れば完成です。

音のファイルやプリセットの名前を間違えると、画面に「開発用のエラー」ウィンドウが出て、見つからない名前と、それを書いた場所 (ファイルと行) が表示されます。
何かが出ないときは、まずこのウィンドウを確認してください。

## エフェクトを調整する

1. 「パーティクルエディター」ウィンドウのプリセットで `gemSparkle` を選ぶ
2. 色・大きさ・数・寿命などを変える。変えた内容はすぐ画面に反映される (ジェムを拾って確かめる)
3. 「セーブ」を押すと `gemSparkle.json` に保存される

画像を使った火花や煙を作るときは、画像を `Assets/Textures/Particles/` に置くと、エディターのテクスチャの選択に出てきます。
別のエフェクトを作るときは、`gemSparkle.json` をコピーして名前を変え、起動し直してから調整してください。

## おつかれさまでした

これで、ゲーム作りに毎回使う流れを一通り体験しました。

| やったこと | 使ったもの |
| :--- | :--- |
| シーンを作って登録した | `BaseScene`、`SceneManager`、`Environment` |
| 置くものを作った | `GameObject`、`objectManager_.Create` |
| モデルを出して動かした | `Model`、`GameInput`、`TimeManager` |
| 値を調整して保存した | `PropertyBinder` |
| 拾う処理を作った | `AddSphereCollider`、`OnCollisionEnter`、レイヤー、タグ |
| 文字を出した | `Text`、`UIAnchor` |
| 音とエフェクトを付けた | `AudioPlayer`、`ParticleSystem` |

次は、作りたいゲームに合わせて [機能別ガイド](../guides/index.md) を読んでください。
たとえば、キャラクターのアニメーションは [3D モデルとアニメーション](../guides/model.md)、タイトル画面からの切り替えは [シーン](../guides/scene.md) にあります。
