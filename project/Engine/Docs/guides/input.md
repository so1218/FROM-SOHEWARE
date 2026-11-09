# 入力

ゲームの操作は `GameInput` で読みます。「ジャンプ」「射撃」のような**操作の名前**で読むので、キーボードとパッドの両方に対応でき、割り当ての変更も 1 か所で済みます。

## 操作を読む

```cpp
#include "GameInput.h"

if (GameInput::IsTriggered(GameAction::Jump)) { /* 押した瞬間 */ }
if (GameInput::IsPressed(GameAction::Sprint)) { /* 押している間 */ }
if (GameInput::IsReleased(GameAction::Aim))   { /* 離した瞬間 */ }

// 移動 (WASD・左スティック)。x が右、y が奥。長さが入力の強さ (0 ~ 1)
Vector2 move = GameInput::GetMoveAxis();

// 視点 (矢印キー・右スティック)。各軸 -1 ~ 1。引数はスティックの遊び (0 ~ 1)
Vector2 look = GameInput::GetLookAxis(0.2f);
```

| 関数 | 内容 |
| :--- | :--- |
| `IsTriggered` | 割り当てのどれかを押した瞬間だけ true |
| `IsPressed` | 割り当てのどれかを押している間 true |
| `IsReleased` | 割り当てをすべて離した瞬間だけ true |
| `GetMoveAxis` / `GetLookAxis` | 移動・視点の向き |
| `GetMenuVertical` | メニューの上下 (上が +1)。押しっぱなしで進めるときは `RepeatInput` に渡す |
| `IsUsingController` | 最後に触ったのがパッドか。ボタンのアイコンを切り替えるときに使う |
| `StartVibration` | パッドを振動させる |

## 操作を足す

割り当ては `Game/Global/GameInput.cpp` の 1 か所にまとめています。

1. `Game/Global/GameInput.h` の `GameAction` に名前を足す
2. `GameInput.cpp` の `kBindings` に、割り当てるキーやボタンを足す

```cpp
// GameInput.h
enum class GameAction
{
    // ...
    Interact,   // 追加
};

// GameInput.cpp
{ GameAction::Interact, { Key(DIK_E), Pad(PadButton::Y) } },
```

キーは DirectInput のキーコード (`DIK_` で始まる名前)、パッドは `Input::PadButton` (Xbox コントローラーのボタン名) で指定します。マウスのボタンは `Mouse(Input::MouseButton::Left)` です。
1 つの操作に、いくつでも割り当てられます。

:::{note}
`GameAction` を足したのに `kBindings` に書き忘れると、その操作を読んだときに assert で止まります。
:::

## キーやボタンを直接読む

マウスの位置やホイールなど、操作の名前を付けるほどでもないものは `Input` を直接使います。

```cpp
#include "Input.h"

const Input& input = Input::GetInstance();
input.IsKeyTriggered(DIK_F2);
input.IsMouseButtonPressed(Input::MouseButton::Right);
Vector2 delta = input.GetMouseDelta();     // このフレームでマウスが動いた量
float wheel = input.GetMouseWheel();       // 奥に回すと正
input.IsPadButtonTriggered(Input::PadButton::A);
```

## メニューのカーソル送り

押しっぱなしにしたときに一定間隔でカーソルを進めるには `RepeatInput` を使います。

```cpp
FS::RepeatInput upRepeat_;   // メンバー

const float vertical = GameInput::GetMenuVertical();
// Windows.h の max マクロとぶつからないよう、std::max はかっこで囲む
if (upRepeat_.Update((std::max)(vertical, 0.0f), deltaTime))
{
    MoveCursorUp();   // 押した瞬間と、押し続けている間の一定間隔ごとに呼ばれる
}
```

## つまずきやすい点

- **ゲームの操作に `Input` を直接使わない。** キーボードしか対応できず、割り当てがあちこちに散らばる
- **メニューを開いている間はプレイヤーの操作を止める。** 同じボタンが両方に効いてしまう。`PlayScene` では、ポーズ中は `Player::SetControllable(false)` で止めている
