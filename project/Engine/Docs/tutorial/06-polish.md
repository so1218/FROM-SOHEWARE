# 6. 音とエフェクトを付ける

ジェムを拾ったときに、効果音を鳴らして光のエフェクトを出します。

## 効果音のファイルを置く

好きな効果音を `gemGet.mp3` という名前で `Assets/Audio/SE/` に置きます (`.wav` でも構いません)。
`Assets/` の中に置いたファイルは起動したときに自動で読み込まれ、ファイル名 (拡張子なし) で使えるようになります。

## 鳴らして、エフェクトを出す

エフェクト (パーティクル) は、`Assets/Data/Particle/` にあるプリセットの名前でエミッター (発生源) を作り、好きなときに `Play()` します。
今回は、もともとある `floatingDamage` を使います。

`Ball.h` に、エミッターを覚えておく変数を足します。

```cpp
#include "ParticleEmitter.h"   // 追加

private:
    // ...
    FS::ParticleEmitter* pickupEffect_ = nullptr;   // 追加。持ち主は ParticleSystem
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
    pickupEffect_ = engine_->GetParticleSystem()->SpawnEmitter("floatingDamage");
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

ジェムを拾うと、音が鳴って光が出れば完成です。

もし音のファイルを置き忘れていると、画面に「開発用のエラー」ウィンドウが出て、見つからない名前と書いた場所が表示されます。
名前の書き間違いもこのウィンドウで分かるので、何かが表示されないときはまずここを確認してください。

## エフェクトを自分で作る

1. `Assets/Data/Particle/floatingDamage.json` をコピーして、`gemSparkle.json` などの名前で同じフォルダに置く
2. 起動して「パーティクルエディター」ウィンドウのプリセットで `gemSparkle` を選び、色や数を調整して「セーブ」を押す
3. コードの `SpawnEmitter("floatingDamage")` を `SpawnEmitter("gemSparkle")` に変える

## おつかれさまでした

これで、ゲーム作りに毎回使う流れを一通り体験しました。

| やったこと | 使ったもの |
| :--- | :--- |
| シーンを作って登録した | `BaseScene`、`SceneManager` |
| 置くものを作った | `GameObject`、`objectManager_.Create` |
| モデルを出して動かした | `Model`、`GameInput`、`TimeManager` |
| 値を調整して保存した | `PropertyBinder` |
| 拾う処理を作った | `AddSphereCollider`、`OnCollisionEnter`、レイヤー |
| 文字を出した | `Text`、`UIAnchor` |
| 音とエフェクトを付けた | `AudioPlayer`、`ParticleSystem` |

次は、作りたいゲームに合わせて [機能別ガイド](../guides/index.md) を読んでください。
たとえば、キャラクターのアニメーションは [3D モデルとアニメーション](../guides/model.md)、タイトル画面からの切り替えは [シーン](../guides/scene.md) にあります。
