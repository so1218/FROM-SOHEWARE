#pragma once
#include "GameObject.h"
#include "Sprite.h"
#include "GlobalVariables.h"

class Engine;

class GameTimer : public GameObject
{
public:
    GameTimer(Engine* engine);
    ~GameTimer() override = default;

    // 初期化 (limitMinutes: 制限時間・分)
    void Initialize(float limitMinutes);

    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // GameObjectTypeはUI扱いにする
    GameObjectType GetType() const override { return GameObjectType::UI; }

    // タイムアップしたか
    bool IsTimeUp() const { return isTimeUp_; }

    // 残り時間を取得（必要なら）
    float GetRemainingTime() const { return currentTime_; }

    // GlobalVariables用
    std::vector<std::string> GetGlobalVariableGroupName() const { return { "GameTimer" }; }
    void ApplyGlobalVariables();

private:
    Engine* engine_ = nullptr;

    // 時間管理
    float maxTime_ = 0.0f;     // 最大時間（秒換算）
    float currentTime_ = 0.0f; // 現在の残り時間
    bool isTimeUp_ = false;

    // 表示用スプライト
    // [0]:分10の位, [1]:分1の位, [2]:コロン, [3]:秒10の位, [4]:秒1の位
    std::array<std::unique_ptr<Sprite>, 5> sprites_;

    // 数字テクスチャのハンドル配列 (0~9)
    std::array<uint32_t, 10> digitTextureHandles_;
    uint32_t colonTextureHandle_ = 0; // コロン(:)用のテクスチャ

    // 調整用パラメータ
    Vector2 position_ = { 640.0f, 50.0f };
    Vector2 charSize_ = { 32.0f, 64.0f };
    float charSpacing_ = 30.0f; // 文字間隔
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 表示内容を更新するヘルパー関数
    void UpdateSpriteTextures(int minutes, int seconds);
};