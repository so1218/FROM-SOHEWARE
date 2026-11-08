#pragma once
#include "GameObject.h"
#include "Model.h"
#include "PropertyBinder.h"

namespace FS { class Terrain; }
class Player;

// 敵を倒したときに散らばるお金。拾うとプレイヤーの所持金になる
// 数が多いので 1 枚ずつは GameObject にせず、このクラスがまとめて動かして描く
class CoinManager : public FS::GameObject
{
public:
    CoinManager(FS::Engine* engine, Player* player, const FS::Terrain* terrain);
    ~CoinManager() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // position から 1 回分のお金を、少しずつ時間をずらして飛び散らせる
    void SpawnBurst(const FS::Vector3& position);

private:
    enum class State
    {
        Waiting,     // 飛び出す順番を待っている
        Flying,      // 飛び散って落ちている
        Resting,     // 地面で回っている
        Attracted,   // プレイヤーに吸い寄せられている
    };

    struct Coin
    {
        std::unique_ptr<FS::Model> model;
        State state = State::Waiting;
        FS::Vector3 position;
        FS::Vector3 velocity;
        float spawnDelay = 0.0f;
        float elapsedTime = 0.0f;   // 飛び出してからの時間
        float spinAngle = 0.0f;
        float attractSpeed = 0.0f;  // 吸い寄せられている速さ。だんだん速くなる
        int burstId = 0;            // 同じ敵から出たものは同じ番号。光を 1 つにまとめるのに使う
        bool isCollected = false;
    };

    struct Config
    {
        int coinsPerBurst = 10;
        int valuePerCoin = 3;
        float spawnDuration = 0.2f;     // 1 回分を出し切るまで (秒)
        float ejectSpeed = 6.0f;        // 上に飛び出す速さ
        float spreadSpeed = 3.0f;       // 横に散らばる速さ
        float gravity = 25.0f;
        float bounce = 0.4f;            // 地面で跳ね返る割合
        float restSpeed = 1.5f;         // これより遅く地面に当たったら止める
        float spinSpeed = 3.0f;         // 地面で回る速さ (ラジアン / 秒)
        float magnetDelay = 0.3f;       // 飛び出してから吸い寄せ始めるまで
        float magnetRadius = 4.0f;
        float magnetAcceleration = 60.0f;
        float magnetMaxSpeed = 30.0f;
        float collectRadius = 0.8f;
        float lifeTime = 30.0f;
        float blinkTime = 3.0f;         // 消える前に点滅する時間
        FS::Vector3 scale = { 0.35f, 0.06f, 0.35f };

        // 散らばった 1 回分ごとに点光源を 1 つ置く
        FS::Vector4 lightColor = { 1.0f, 0.8f, 0.3f, 1.0f };
        float lightIntensity = 5.0f;
        float lightRadius = 6.0f;
        float lightVolumetric = 1.0f;
        float lightHeight = 0.5f;
        int maxLitBursts = 3;           // プレイヤーに近いものから、この数だけ照らす
    };

    // プレイヤーの当たり判定の箱。お金を吸い寄せたり拾ったりするのに使う
    struct PlayerArea
    {
        FS::Vector3 center;
        FS::Vector3 halfSize;
    };

    void UpdateCoin(Coin& coin, float deltaTime, const PlayerArea& playerArea);
    bool IsInsidePlayer(const FS::Vector3& position, const PlayerArea& playerArea) const;
    void UpdateFlying(Coin& coin, float deltaTime);
    void Collect(Coin& coin);
    void SubmitBurstLights(const FS::Vector3& playerCenter);
    bool IsHiddenByBlink(const Coin& coin) const;

    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;

    // マテリアルの共有元。ImGui で色や発光を編集する
    std::unique_ptr<FS::Model> sourceModel_;
    std::unique_ptr<FS::PropertyBinder> binder_;
    Config config_;

    std::vector<Coin> coins_;
    int nextBurstId_ = 0;
    float elapsedTime_ = 0.0f;
    std::mt19937 random_;
};
