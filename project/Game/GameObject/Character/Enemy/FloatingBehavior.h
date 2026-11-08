#pragma once
#include "Engine.h"
#include "IEnemyBehavior.h"
#include "PropertyBinder.h"
#include "Model.h"
#include <random>

namespace FS { class Terrain; }

class Player;

// 浮遊する敵で共通の設定
struct FloatingEnemyConfig
{
    void Bind(FS::PropertyBinder& binder);
    void Draw(FS::PropertyBinder& binder);

    EnemyBodyConfig body = { 100, { 2.0f, 2.0f, 2.0f }, { 6.5f, 6.5f, 6.5f }, { 0.0f, 2.0f, 0.0f } };
    float turnSpeed = 6.0f;        // 機体の向きが変わる速さ

    // 発見
    float sightRange = 60.0f;      // この距離まで近づくと気づく
    float hearingRange = 120.0f;   // この距離の銃声に気づく
    float loseRange = 150.0f;      // これより離れると見失って巡回に戻る

    // 追跡
    float moveSpeed = 18.0f;
    float acceleration = 3.0f;     // 大きいほど早く目標の速さになる
    float keepDistance = 25.0f;    // プレイヤーとの水平距離
    float keepHeight = 10.0f;      // プレイヤーより上の高さ
    float orbitSpeed = 0.6f;       // 周回の速さ (ラジアン / 秒)
    float wobbleAmount = 2.0f;     // ふらつきの大きさ

    // 攻撃
    float attackInterval = 2.5f;   // 追跡に入ってから狙い始めるまで (秒)
    float lockOnTime = 1.0f;       // レーザーで狙う時間 (秒)
    int burstCount = 6;            // 1 回の連射で撃つ数
    float shotInterval = 0.1f;     // 連射の間隔 (秒)
    int damagePerShot = 4;
    float shotSpread = 0.03f;      // ばらつき (ラジアン)
    float maxFireDistance = 80.0f;
    FS::Vector3 muzzleOffset = { 0.0f, 2.0f, 0.0f };

    // スラスター (弱点)。位置と大きさには機体のスケールがかかる
    FS::Vector3 thrusterOffset = { 1.0f, 1.0f, 1.0f };   // 1 基目の位置。残りは x, z の符号を変えた位置
    FS::Vector3 thrusterScale = { 0.5f, 0.5f, 0.5f };
    FS::Vector3 thrusterColliderSize = { 1.0f, 1.0f, 1.0f };
    int thrusterMaxHP = 30;
    float bodyDamageRate = 0.3f;   // 本体に当たったときのダメージの倍率
    int thrustersToCrash = 2;      // この数を壊すと墜落する

    // 墜落
    float crashTime = 5.0f;        // 地面に届かなくても、この時間で爆発する
    float crashGravity = 30.0f;

    // スポットライト
    FS::Vector4 spotColor = { 0.076f, 0.216f, 1.0f, 1.0f };
    float spotIntensity = 30.85f;
    float spotDistance = 28.24f;
    float spotAngleDeg = 28.13f;
    float spotVolumetric = 200.0f;
    FS::Vector3 spotLocalDirection = { 0.0f, -0.3f, -1.0f };   // 機体から見た向き。-Z が正面
};

// 空中を飛ぶドローン型の敵。プレイヤーを見つけると距離を保って周りを回り、レーザーで狙ってから連射する。
// 本体は硬く、4 基のスラスターのうち決まった数を壊すと墜落する
class FloatingBehavior : public IEnemyBehavior
{
public:
    // thrusterSourceModel はスラスターのマテリアルの共有元
    FloatingBehavior(const FloatingEnemyConfig& config, const FS::Terrain* terrain, const FS::Model* thrusterSourceModel);
    ~FloatingBehavior() override;

    void Initialize(Enemy* owner) override;
    void Update(Enemy* owner) override;
    void DebugGui(Enemy* owner) override;
    void Draw(Enemy* owner) override;

    const EnemyBodyConfig& GetBodyConfig() const override { return config_.body; }
    std::string GetDamageParticleName() const override { return "floatingDamage"; }

    int ModifyDamage(Enemy* owner, int damage, const FS::Collider* hitCollider) override;
    void OnDeath(Enemy* owner) override;
    void OnDespawn(Enemy* owner) override;
    bool OnHealthDepleted(Enemy* owner) override;

private:
    enum class State
    {
        Patrol,   // 巡回
        Chase,    // プレイヤーの周りを回りながら近づく
        LockOn,   // レーザーで狙う
        Fire,     // 連射
        Crash,    // 墜落
    };

    static constexpr int kThrusterCount = 4;

    // 状態ごとの更新
    void UpdatePatrol(Enemy* owner, float deltaTime);
    void UpdateChase(Enemy* owner, float deltaTime);
    void UpdateLockOn(Enemy* owner, float deltaTime);
    void UpdateFire(Enemy* owner, float deltaTime);
    void UpdateCrash(Enemy* owner, float deltaTime);
    void ChangeState(State state);
    void StartChase(Enemy* owner);
    void StartCrash();

    // 移動と向き
    bool CanDetectPlayer(const FS::Vector3& position);
    void MoveToward(Enemy* owner, const FS::Vector3& target, float speed, float deltaTime);
    void FaceDirection(Enemy* owner, const FS::Vector3& direction, float deltaTime);

    // 射撃
    void FireShot(Enemy* owner);
    FS::Vector3 GetMuzzlePosition(Enemy* owner) const;

    // スラスター
    void DestroyThruster(Enemy* owner, int thrusterIndex);
    void RepairThrusters();
    FS::Vector3 GetThrusterLocalPosition(int thrusterIndex) const;
    void UpdateThrusters(Enemy* owner);

    float RandomRange(float min, float max);
    void ReleaseEffects();

    const FloatingEnemyConfig& config_;
    const FS::Model* thrusterSourceModel_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;   // 墜落したときに地面の高さを調べる
    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;

    // 状態
    State state_ = State::Patrol;
    float stateTimer_ = 0.0f;         // 今の状態になってからの時間
    float elapsedTime_ = 0.0f;
    int shotsFired_ = 0;              // 今の連射で撃った数
    int lastPlayerShotCount_ = 0;     // 銃声に気づくために、前のフレームの発射数を覚えておく
    std::mt19937 random_;

    // 移動
    FS::Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    FS::Vector3 aimDirection_ = { 0.0f, 0.0f, 1.0f };   // ロックオンで決めた向き
    float orbitAngle_ = 0.0f;
    float orbitDirection_ = 1.0f;     // 1 = 左回り, -1 = 右回り
    float orbitChangeTimer_ = 0.0f;   // 0 になったら周回の向きを変える

    // スラスター
    int thrusterHP_[kThrusterCount] = {};
    FS::Collider* thrusterColliders_[kThrusterCount] = {};   // 所有は Enemy (AddBoxCollider で作る)
    std::unique_ptr<FS::Model> thrusterModels_[kThrusterCount];
    float propellerAngle_ = 0.0f;
    int destroyedThrusterCount_ = 0;

    // 撃った弾の線を少しだけ残す
    struct Tracer
    {
        FS::Vector3 start;
        FS::Vector3 end;
        float remainingTime;
    };
    std::vector<Tracer> tracers_;

    // 巡回。基準位置を中心にサイン波で動く。1 機ずつ違う値を持つ
    FS::Vector3 basePosition_;
    FS::Vector3 amplitude_ = { 3.0f, 2.0f, 0.0f };
    FS::Vector3 frequency_ = { 0.5f, 1.0f, 0.0f };
    FS::Vector3 phase_ = { 0.0f, 0.0f, 0.0f };   // 度
    float patrolSpeed_ = 30.0f;

    // 演出
    FS::ParticleEmitter* moveFireEmitterPtr_ = nullptr;
    FS::ParticleEmitter* moveSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* explosionEmitterPtr_ = nullptr;
    FS::ParticleEmitter* thrusterBreakEmitterPtr_ = nullptr;
    int soundHandleID_ = -1;
};
