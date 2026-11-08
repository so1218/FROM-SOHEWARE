# 音

効果音と BGM は `FS::AudioPlayer` で鳴らします。`Assets/Audio/` に置いたファイルは起動時に自動で読み込まれ、ファイル名 (拡張子なし) で鳴らせます。

```cpp
#include "AudioPlayer.h"

AudioPlayer& audio = AudioPlayer::GetInstance();
```

## 効果音

```cpp
audio.Play("jump");          // 1 回鳴らす
audio.Play("jump", 0.5f);    // 音量 (0 ~ 1) を指定
```

## 位置のある音

足音・爆発など、鳴った場所から聞こえる音は `Play3D` を使います。カメラから遠いほど小さくなり、左右にも分かれて聞こえます。

```cpp
audio.Play3D("explosion", position);

// 音量や聞こえる距離を変えるとき
audio.Play3D("gunShot", muzzlePosition, { .volume = 0.5f, .maxDistance = 150.0f });
```

| 設定 | 内容 |
| :--- | :--- |
| `volume` | 音量 (0 ~ 1) |
| `maxDistance` | これより遠いと聞こえない |
| `isLoop` | 繰り返すか。エンジン音など鳴り続ける音に使う |

音源が動くときは、返ってきたハンドルで位置を更新します。

```cpp
engineSound_ = audio.Play3D("engine", position, { .isLoop = true });   // Initialize で
audio.SetEmitterPosition(engineSound_, GetTransform().translation_);    // Update で
audio.Stop(engineSound_);                                               // 止めるとき
```

## 繰り返す音と BGM

```cpp
int handle = audio.PlayLoop("rain", 0.3f);   // 止めるまで鳴り続ける
audio.Stop(handle);

audio.PlayBgm("sunnyBGM", 0.4f);   // 同じ BGM が鳴っていれば何もしない
audio.StopBgm("sunnyBGM");
```

`PlayBgm` は、毎フレーム呼んでも最初の 1 回だけ鳴らします。「今の状態で鳴らすべき BGM」を毎フレーム渡すだけで済みます。

## 音のファイル

- 形式は `.mp3` と `.wav`
- `Assets/Audio/` の中なら、どのフォルダに置いても構わない (`SE/` と `BGM/` に分けている)
- 名前はファイル名。フォルダが違っても、同じ名前のファイルは 1 つにする

## つまずきやすい点

- **鳴らない。** 名前を間違えると「開発用のエラー」ウィンドウに名前と場所が出る。ファイルを置いたあとに起動し直したかも確認する
- **繰り返す音が止まらない。** `PlayLoop` / `Play3D` (`isLoop = true`) で返ったハンドルを覚えておき、オブジェクトが消えるとき (`OnDestroy` やシーンを抜けるとき) に `Stop` する
