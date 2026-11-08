# カメラ

各シーンはカメラを 1 つ持っていて (`camera_`)、画面はそのカメラから見たものになります。
カメラを動かす処理は「カメラコントローラー」として作り、`cameraManager_` に渡して切り替えます。

## 動かないカメラ

カメラの位置と向きを決めるだけなら、シーンの `OnInitialize()` で直接設定します。

```cpp
void SampleScene::OnInitialize()
{
    const Vector3 eye = { 0.0f, 12.0f, -14.0f };
    const Vector3 target = { 0.0f, 0.0f, 0.0f };
    camera_->SetTranslation(eye);
    camera_->SetRotation(Quaternion::LookRotation(target - eye, Vector3::Up()));
}
```

シーン名のウィンドウの「メインカメラ設定」でも位置・向き・画角を変えられます。そこで保存した値は、次の起動からコードの値より優先されます。

## 動くカメラ (カメラコントローラー)

プレイヤーを追うカメラなど、毎フレーム動かすものは `FS::ICameraController` を継承して作ります。

```cpp
class TopDownCamera : public FS::ICameraController
{
public:
    explicit TopDownCamera(const FS::WorldTransform* target) : target_(target) {}

    // 毎フレーム呼ばれる。camera の位置と向きを決める
    void UpdateCamera(FS::Camera* camera) override
    {
        const FS::Vector3 eye = target_->translation_ + FS::Vector3{ 0.0f, 10.0f, -8.0f };
        camera->SetTranslation(eye);
        camera->SetRotation(FS::Quaternion::LookRotation(target_->translation_ - eye, FS::Vector3::Up()));
    }

private:
    const FS::WorldTransform* target_ = nullptr;
};
```

シーンで作って、`cameraManager_` に渡します。

```cpp
// メンバー
std::unique_ptr<TopDownCamera> topDownCamera_;

// コンストラクタで
Ball* ball = objectManager_.Create<Ball>(engine_);
topDownCamera_ = std::make_unique<TopDownCamera>(&ball->GetTransform());

// OnInitialize で
cameraManager_->ChangeController(topDownCamera_.get());
```

カメラはオブジェクトの更新と当たり判定が終わったあとで更新されるので、追う相手の最新の位置が使えます。
タイトル画面とゲーム中でカメラを変えるときは、`ChangeController` で別のコントローラーに切り替えます。`nullptr` を渡すとカメラは止まります。

実例は `Game/Camera/FollowCamera.cpp` (プレイヤーを追う三人称カメラ) と `TitleCamera.cpp` (タイトルからゲームへ移るカメラ) です。

## 画面を揺らす

```cpp
// 揺れる時間 (秒), 揺れの強さ
cameraManager_->RequestShake(0.3f, 0.5f);
```

揺れはカメラコントローラーが動かした位置に上乗せされます。**コントローラーを設定していないと揺れません。**

GameObject から揺らしたいときは、シーンが作るときに `CameraManager*` を渡しておきます (例: `WeatherEffectManager` は雷のときに画面を揺らしている)。

## カメラの向きを使う

カメラから見た前後左右に動かすときは、カメラの回転を使います。

```cpp
const Quaternion cameraRotation = camera->GetRotation();
Vector3 forward = cameraRotation.RotateVector({ 0.0f, 0.0f, 1.0f });
forward.y = 0.0f;   // 地面に沿って動かすので、上下の成分を消す
forward = forward.Normalize();
Vector3 right = Vector3::Up().Cross(forward);

const Vector2 move = GameInput::GetMoveAxis();
velocity = (right * move.x + forward * move.y) * moveSpeed;
```

## つまずきやすい点

- **デバッグカメラ (Y キー) で見ている間は、シーンのカメラの映像ではない。** エディター画面で Y を押すと切り替わる。ゲーム画面 (F1) に戻すと必ずシーンのカメラになる
- **保存したカメラ設定がコードより優先される。** コードで位置を変えても反映されないときは、シーンの JSON (`Assets/Data/GlobalVariables/<シーン名>.json`) に保存された値を確認する
