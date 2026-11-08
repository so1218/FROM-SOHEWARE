# 入力・時間・音声・ログ

本ドキュメントでは、ゲームのどこからでも使える共通機能（入力、時間管理、音声再生、ログ出力）の使い方を解説します。
いずれもシングルトンとして提供されているため、`Engine*` を経由せずに直接呼び出せます（[エンジン基本設計ガイド 5 節](engineBasicsGuide.md#5-シングルトン機能との使い分け)）。

---

## 目次
- [1. 入力 (Input)](#1-入力-input)
- [2. 時間管理 (TimeManager)](#2-時間管理-timemanager)
- [3. 音声再生 (AudioPlayer)](#3-音声再生-audioplayer)
- [4. ログ出力 (Logger)](#4-ログ出力-logger)

---

## 1. 入力 (Input)

`FS::Input` は DirectInput (キーボード・マウス) と XInput (Xbox コントローラー、最大 4 台) をまとめて扱います。入力状態は毎フレームの最初に `Game::Run()` から更新されます。

```cpp
#include "Input.h"

auto& input = FS::Input::GetInstance();
```

### 1-1. キーボード

キーは DirectInput のキーコード (`DIK_`) で指定します。

| 関数 | 判定 |
| :--- | :--- |
| `IsKeyTriggered(DIK_SPACE)` | 押した瞬間 |
| `IsKeyPressed(DIK_W)` | 押している間 |
| `IsKeyReleased(DIK_W)` | 離した瞬間 |
| `IsKeyUp(DIK_W)` | 押していない間 |

```cpp
if (input.IsKeyPressed(DIK_W)) { move.z += 1.0f; }
if (input.IsKeyTriggered(DIK_SPACE)) { Jump(); }
```

### 1-2. マウス

```cpp
FS::Vector2 pos = input.GetMousePosition();   // クライアント座標
int wheel = input.GetMouseWheelDelta();       // ホイール移動量

if (input.IsMouseButtonTriggered(FS::Input::Left))  { Shoot(); }
if (input.IsMouseButtonPressed(FS::Input::Right))   { Aim(); }
```

ボタンは `FS::Input::Left` / `Right` / `Middle` で指定します。離した瞬間は `IsMouseButtonIsKeyReleased()`、押していない間は `IsMouseButtonUp()` です。

### 1-3. コントローラー

第 1 引数はコントローラー番号 (0〜3) です。

```cpp
if (input.IsControllerConnected(0))
{
    if (input.IsControllerButtonTriggered(0, FS::Input::ButtonA)) { Jump(); }

    SHORT lx = input.GetLeftStickX(0);  // -32768 〜 32767
    SHORT ly = input.GetLeftStickY(0);

    // 振動 (左右モーターの強さ 0.0〜1.0、0.2 秒)
    input.StartVibration(0, 0.5f, 0.5f, 0.2f);
}
```

- ボタン : `ButtonA` `ButtonB` `ButtonX` `ButtonY` `ButtonLB` `ButtonRB` `ButtonLT` `ButtonRT` `ButtonL3` `ButtonR3` `ButtonMenu` `ButtonPhoto` `ButtonUp` `ButtonDown` `ButtonLeft` `ButtonRight`
- スティックを方向キーのように使う判定 (`IsStickUpTriggered(0, FS::Input::LeftStick)` など) もあり、メニュー操作に便利です。しきい値は `STICK_THRESHOLD` (0x4000) です。

---

## 2. 時間管理 (TimeManager)

`FS::TimeManager` はフレーム間の経過時間 (デルタタイム) を管理します。移動量やタイマーは、フレームレートに依存しないようにデルタタイムを掛けて計算します。

```cpp
#include "TimeManager.h"

auto* time = FS::TimeManager::GetInstance();

float dt = time->GetDeltaTime();         // タイムスケール適用済みの経過秒数
position += velocity * dt;
```

| 関数 | 内容 |
| :--- | :--- |
| `GetDeltaTime()` | 前フレームからの経過秒数 × タイムスケール |
| `GetUnscaledDeltaTime()` | タイムスケールの影響を受けない経過秒数 (UI 演出など) |
| `GetTotalTime()` | 起動からの経過秒数 |
| `GetFPS()` / `GetAverageFPS()` | 現在 / 平均の FPS |
| `SetTimeScale(float)` | 時間の進む速さ (0.0〜10.0)。スローモーションなど |
| `Pause()` / `Resume()` / `IsPaused()` | ポーズ。ポーズ中は `SceneManager::Update()` が呼ばれない |

目標 FPS は `ProjectConfig::targetFPS` で設定し、`FrameLimiter` がフレームの終わりに待機して調整します。

---

## 3. 音声再生 (AudioPlayer)

`FS::AudioPlayer` は XAudio2 を使って音声を再生します。音声ファイルは起動時に `Assets/Data/AudioList.csv` の内容に従って一括で読み込まれ、CSV に書いた **名前** で再生できます（登録方法は [アセットとデータファイル](assetsGuide.md#3-音声-audiolistcsv)）。

```cpp
#include "AudioPlayer.h"

auto& audio = FS::AudioPlayer::GetInstance();
```

### 3-1. 効果音 (SE)

```cpp
// 名前, ループするか, 音量 (0〜100)
int handle = audio.Play("gunShot", false, 20);

audio.Stop(handle);        // 個別に止める
audio.IsPlaying(handle);   // 再生中か
```

### 3-2. BGM (同じ音を重ねない再生)

`PlayUnique()` は、同じ名前の音がすでに再生中なら新たに再生しません。BGM や環境音のように、毎フレーム呼んでも 1 つだけ鳴らしたい音に使います。

```cpp
audio.PlayUnique("sunnyBGM", true, 50);
audio.StopUnique("sunnyBGM");
```

### 3-3. 3D サウンド

位置を指定して再生すると、カメラ (リスナー) との距離と向きから音量と左右のバランスが毎フレーム自動計算されます。リスナーの位置は `Engine::SetCameraState()` が自動で設定します。

```cpp
// 名前, 位置, ループ, 音量, 聞こえる最大距離
int handle = audio.Play3D("lightningStrike", strikePosition, false, 50, 100.0f);

// 動く音源は位置を更新する
audio.SetEmitterPosition(handle, newPosition);
```

`maxDistance` 以上離れると音量は 0 になります。

---

## 4. ログ出力 (Logger)

ログはマクロで出力します。書式は `std::format` と同じで、出力にはファイル名と行番号が自動で付きます。

```cpp
#include "Logger.h"

LOG_DEBUG("enemy count = {}", enemies.size());
LOG_INFO("Scene changed: {}", sceneName);
LOG_WARN("Texture not found: {}", name);
LOG_ERROR("Failed to load {}", path);
```

- 出力先はコンソールと `project/logs/<日時>.log` です。
- `ENABLE_DEV_TOOLS` が定義されていないビルド (Release) では、`LOG_ERROR` 以外は何もしないマクロになります。
- 出力するレベルの下限は `FS::Logger::Instance().SetLogLevel(FS::LogLevel::Debug)` で変更できます (既定は `Info`)。
- アプリケーションがクラッシュすると、`project/Dumps/` にクラッシュダンプ (`.dmp`) が書き出されます。Visual Studio で開くと、クラッシュした位置を確認できます。
