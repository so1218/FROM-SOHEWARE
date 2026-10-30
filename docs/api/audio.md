# オーディオ (Audio)

XAudio2 による音声再生 (2D/3D) と、Media Foundation によるデコードです。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::AudioData <fs-audiodata>` | struct | `Engine/Audio/AudioDefinition.h` | 音声データ |
| {ref}`FS::AudioDevice <fs-audiodevice>` | class | `Engine/Audio/AudioDevice.h` |  |
| {ref}`FS::AudioInstance <fs-audioinstance>` | struct | `Engine/Audio/AudioPlayer.h` | @brief 再生中のインスタンス情報 |
| {ref}`FS::AudioPlayer <fs-audioplayer>` | class | `Engine/Audio/AudioPlayer.h` | @brief 音声再生管理クラス |
| {ref}`FS::VoiceCallback <fs-voicecallback>` | class | `Engine/Audio/AudioPlayer.h` | コールバックは変更なしでOK |
| {ref}`FS::MediaAudioDecoder <fs-mediaaudiodecoder>` | class | `Engine/Audio/MediaAudioDecoder.h` |  |

(fs-audiodata)=
## FS::AudioData

音声データ

ヘッダー: `Engine/Audio/AudioDefinition.h`

```{doxygenstruct} FS::AudioData
:members:
:protected-members:
:undoc-members:
```

(fs-audiodevice)=
## FS::AudioDevice

ヘッダー: `Engine/Audio/AudioDevice.h`

```{doxygenclass} FS::AudioDevice
:members:
:protected-members:
:undoc-members:
```

(fs-audioinstance)=
## FS::AudioInstance

@brief 再生中のインスタンス情報

ヘッダー: `Engine/Audio/AudioPlayer.h`

```{doxygenstruct} FS::AudioInstance
:members:
:protected-members:
:undoc-members:
```

(fs-audioplayer)=
## FS::AudioPlayer

@brief 音声再生管理クラス

ヘッダー: `Engine/Audio/AudioPlayer.h`

```{doxygenclass} FS::AudioPlayer
:members:
:protected-members:
:undoc-members:
```

(fs-voicecallback)=
## FS::VoiceCallback

コールバックは変更なしでOK

ヘッダー: `Engine/Audio/AudioPlayer.h`

```{doxygenclass} FS::VoiceCallback
:members:
:protected-members:
:undoc-members:
```

(fs-mediaaudiodecoder)=
## FS::MediaAudioDecoder

ヘッダー: `Engine/Audio/MediaAudioDecoder.h`

```{doxygenclass} FS::MediaAudioDecoder
:members:
:protected-members:
:undoc-members:
```
