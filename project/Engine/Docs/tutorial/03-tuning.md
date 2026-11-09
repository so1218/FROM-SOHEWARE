# 3. 値をエディターで調整する

ボールの速さを、ゲームを動かしたままエディター画面で変えられるようにして、気に入った値を保存します。
数値をコードに直接書いて、変えるたびにビルドし直す必要がなくなります。

## PropertyBinder を使う

`FS::PropertyBinder` に変数を登録すると、次の 3 つがまとめてできるようになります。

- エディター画面で値を変える
- 値を JSON に保存する
- 起動したときに、保存した値を読み込む

`Ball.h` に PropertyBinder と、エディター画面を出す `DebugGui()` を足します。

```cpp
#include "PropertyBinder.h"   // 追加

class Ball : public FS::GameObject
{
public:
    // ...
    void DebugGui() override;   // 追加

private:
    // ...
    std::unique_ptr<FS::PropertyBinder> binder_;   // 追加
};
```

`Ball.cpp` を次のように変えます。

```cpp
Ball::Ball(Engine* engine) : engine_(engine)
{
    model_ = std::make_unique<Model>(engine_, "sphere");

    // "Ball" は保存先の名前。Assets/Data/GlobalVariables/Ball.json に保存される
    binder_ = std::make_unique<PropertyBinder>(engine_, "Ball");
}

void Ball::Initialize()
{
    GetTransform().translation_ = { 0.0f, 1.0f, 0.0f };

    // 保存した値があれば読み込み、なければ 6.0 から始める。
    // 後ろの 3 つは、ドラッグしたときの変化量・最小値・最大値
    binder_->Bind("MoveSpeed", &moveSpeed_, 6.0f, 0.1f, 0.0f, 50.0f);
}

void Ball::DebugGui()
{
    ImGui::Begin("ボール");
    binder_->Draw("MoveSpeed", "移動の速さ");
    ImGui::End();
}
```

- `Bind` で「どの変数を、どのキー名で保存するか」を決める
- `Draw` で、登録したものを ImGui のウィンドウに出す。2 つ目の引数は画面に出す名前
- `DebugGui()` はエディター画面のときだけ呼ばれる。ゲーム画面 (F1) では呼ばれない

## 動かしてみる

1. 起動すると「ボール」ウィンドウが出る
2. 「移動の速さ」をドラッグすると、ボールの速さがすぐ変わる
3. 「パラメータの保存」ウィンドウの「すべての変更を保存」を押す
4. ゲームを終了して起動し直しても、変えた速さのまま

:::{warning}
保存したキー名 (`"MoveSpeed"`) を後から変えると、保存した値は読まれなくなります。名前を変えるときは JSON のキーも一緒に書き換えてください。
:::

## 詳しく知りたいとき

- [パラメータの調整と保存](../guides/parameters.md): 色・回転・モデルの見た目・UI の位置の登録、保存の仕組み
