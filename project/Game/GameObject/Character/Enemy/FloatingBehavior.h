#pragma once
#include "Engine.h"
#include "IEnemyBehavior.h"
#include "PropertyBinder.h"
#include "Model.h"
#include <random>

namespace FS { class Terrain; }

class Player;

// ワスプ全体で共通のパラメータ。EnemyManager が 1 つ持ち、全機が参照する
struct FloatingEnemyConfig
{
    void Bind(FS::PropertyBinder& binder);
    void Draw(FS::PropertyBinder& binder);

    EnemyBodyConfig body = { 100, { 2.0f, 2.0f, 2.0f }, { 3.25f, 3.25f, 3.25f }, { 0.0f, 2.0f, 0.0f } };
    float turnSpeed = 6.0f;

    // 発見
    float sightRange = 60.0f;      // この距離まで近づくと気づく
    float hearingRange = 120.0f;   // この距離の銃声に気づく
    float loseRange = 150.0f;      // これより離れると見失って巡回に戻る

    // 追跡
    float moveSpeed = 18.0f;
    float acceleration = 3.0f;     // 大きいほど速く向きを変える
    float keepDistance = 25.0f;    // プレイヤーとの水平距離
    float keepHeight = 10.0f;      // プレイヤーより上の高さ
    float orbitSpeed = 0.6f;       // 周回の速さ (ラジアン / 秒)
    float wobbleAmount = 2.0f;     // 不規則なふらつき
    float attackInterval = 2.5f;   // 攻撃と攻撃の間

    // 攻撃
    float lockOnTime = 1.0f;
    int burstCount = 6;
    float shotInterval = 0.1f;
    int damagePerShot = 4;
    float shotSpread = 0.03f;      // ばらつき (ラジアン)
    float maxFireDistance = 80.0f;
    FS::Vector3 muzzleOffset = { 0.0f, 2.0f, 0.0f };

    // 弱点。位置と大きさは機体のローカル単位 (機体のスケールがかかる)
    FS::Vector3 thrusterOffset = { 1.0f, 1.0f, 1.0f };  // 1 基目の位置。残りは x, z の符号を変えた位置
    FS::Vector3 thrusterScale = { 0.5f, 0.5f, 0.5f };
    FS::Vector3 thrusterColliderSize = { 0.5f, 0.5f, 0.5f };   // 当たり判定の箱の半分の大きさ
    int thrusterMaxHP = 30;
    float bodyDamageRate = 0.3f;   // 本体に当たったときのダメージの倍率
    int thrustersToCrash = 2;

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

// ARC Raiders のワスプのような飛行ドローン。見つけると距離を保って周回し、レーザーで狙ってから連射する
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

    void UpdatePatrol(Enemy* owner, float deltaTime);
    void UpdateChase(Enemy* owner, float deltaTime);
    void UpdateLockOn(Enemy* owner, float deltaTime);
    void UpdateFire(Enemy* owner, float deltaTime);
    void UpdateCrash(Enemy* owner, float deltaTime);

    void ChangeState(State state);
    void StartChase(Enemy* owner);
    bool CanDetectPlayer(const FS::Vector3& position);
    void MoveToward(Enemy* owner, const FS::Vector3& target, float speed, float deltaTime);
    void FaceDirection(Enemy* owner, const FS::Vector3& direction, float deltaTime);
    void FireShot(Enemy* owner);
    void DestroyThruster(Enemy* owner, int thrusterIndex);
    void RepairThrusters();
    void StartCrash();
    FS::Vector3 GetMuzzlePosition(Enemy* owner) const;
    float RandomRange(float min, float max);
    void ReleaseEffects();
    FS::Vector3 GetThrusterLocalPosition(int thrusterIndex) const;
    void UpdateThrusters(Enemy* owner);

    const FloatingEnemyConfig& config_;
    const FS::Model* thrusterSourceModel_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;   // 墜落したときに地面の高さを調べる
    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;

    State state_ = State::Patrol;
    float stateTimer_ = 0.0f;
    float elapsedTime_ = 0.0f;
    float orbitChangeTimer_ = 0.0f;   // 0 になったら周回の向きを変える
    FS::Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    FS::Vector3 aimDirection_ = { 0.0f, 0.0f, 1.0f };   // ロックオン時に決めた向き
    float orbitAngle_ = 0.0f;
    float orbitDirection_ = 1.0f;     // 1 = 左回り, -1 = 右回り
    int shotsFired_ = 0;
    int lastPlayerShotCount_ = 0;
    std::mt19937 random_;

    int thrusterHP_[kThrusterCount] = {};
    FS::Collider* thrusterColliders_[kThrusterCount] = {};   // 所有は Enemy (AddCollider で作る)
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

    // 巡回 (1 機ずつ違う。基準位置を中心にサイン波で動く)
    FS::Vector3 basePosition_;
    FS::Vector3 amplitude_ = { 3.0f, 2.0f, 0.0f };
    FS::Vector3 frequency_ = { 0.5f, 1.0f, 0.0f };
    FS::Vector3 phase_ = { 0.0f, 0.0f, 0.0f };
    float patrolSpeed_ = 30.0f;

    // 演出
    FS::ParticleEmitter* moveFireEmitterPtr_ = nullptr;
    FS::ParticleEmitter* moveSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* explosionEmitterPtr_ = nullptr;
    FS::ParticleEmitter* thrusterBreakEmitterPtr_ = nullptr;
    int soundHandleID_ = -1;
};
