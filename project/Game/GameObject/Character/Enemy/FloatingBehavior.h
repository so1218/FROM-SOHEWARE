#pragma once
#include "Engine.h"
#include "IEnemyBehavior.h"
#include <random>

class Player;

// ARC Raiders のワスプのような飛行ドローン。
// 巡回 → プレイヤーを見つける (視界・銃声) → 距離を保って周回 → レーザーで狙う → 短く連射。
// 4 基のスラスターが弱点で、決まった数を壊されると墜落する
class FloatingBehavior : public IEnemyBehavior
{
public:
    FloatingBehavior();
    ~FloatingBehavior() override;

    void Initialize(Enemy* owner) override;
    void Update(Enemy* owner) override;
    void DebugDraw(Enemy* owner) override;

    int GetInitialHP() const override { return 100; }
    std::string GetDamageParticleName() const override { return "floatingDamage"; }

    int ModifyDamage(Enemy* owner, int damage, const FS::Vector3& hitPoint) override;
    void OnDeath(Enemy* owner) override;

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
    bool CanDetectPlayer(const FS::Vector3& position);
    void MoveToward(Enemy* owner, const FS::Vector3& target, float speed, float deltaTime);
    void FaceDirection(Enemy* owner, const FS::Vector3& direction, float deltaTime);
    void FireShot(Enemy* owner);
    void DestroyThruster(Enemy* owner, int thrusterIndex);
    FS::Vector3 GetMuzzlePosition(Enemy* owner) const;
    float RandomRange(float min, float max);
    FS::Vector3 GetThrusterLocalPosition(int thrusterIndex) const;
    void DrawThrusterMarkers(Enemy* owner);

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

    // 弱点
    int thrusterHP_[kThrusterCount] = {};
    int destroyedThrusterCount_ = 0;

    // 撃った弾の線を少しだけ残す
    struct Tracer
    {
        FS::Vector3 start;
        FS::Vector3 end;
        float remainingTime;
    };
    std::vector<Tracer> tracers_;

    // ---- パラメータ ----
    // 巡回 (基準位置を中心にサイン波で動く)
    FS::Vector3 basePosition_;
    FS::Vector3 amplitude_ = { 3.0f, 2.0f, 0.0f };
    FS::Vector3 frequency_ = { 0.5f, 1.0f, 0.0f };
    FS::Vector3 phase_ = { 0.0f, 0.0f, 0.0f };

    // 発見
    float sightRange_ = 60.0f;      // この距離まで近づくと気づく
    float hearingRange_ = 120.0f;   // この距離の銃声に気づく
    float loseRange_ = 150.0f;      // これより離れると見失って巡回に戻る

    float patrolSpeed_ = 30.0f;
    float turnSpeed_ = 6.0f;

    // 追跡
    float moveSpeed_ = 18.0f;
    float acceleration_ = 3.0f;     // 大きいほど速く向きを変える
    float keepDistance_ = 25.0f;    // プレイヤーとの水平距離
    float keepHeight_ = 10.0f;      // プレイヤーより上の高さ
    float orbitSpeed_ = 0.6f;       // 周回の速さ (ラジアン / 秒)
    float wobbleAmount_ = 2.0f;     // 不規則なふらつき
    float attackInterval_ = 2.5f;   // 攻撃と攻撃の間

    // 攻撃
    float lockOnTime_ = 1.0f;
    int burstCount_ = 6;
    float shotInterval_ = 0.1f;
    int damagePerShot_ = 4;
    float shotSpread_ = 0.03f;      // ばらつき (ラジアン)
    float maxFireDistance_ = 80.0f;
    FS::Vector3 muzzleOffset_ = { 0.0f, 2.0f, 0.0f };

    // 弱点
    // 位置と半径はモデルのローカル単位 (スケールがかかる)
    FS::Vector3 thrusterOffset_ = { 1.0f, 1.0f, 1.0f };  // 1 基目の位置。残りは x, z の符号を変えた位置
    float thrusterRadius_ = 0.8f;
    int thrusterMaxHP_ = 30;
    float bodyDamageRate_ = 0.3f;   // 本体に当たったときのダメージの倍率
    int thrustersToCrash_ = 2;
    bool showThrusters_ = false;

    // 墜落
    float crashTime_ = 2.0f;
    float crashGravity_ = 30.0f;

    // 演出
    FS::ParticleEmitter* moveFireEmitterPtr_ = nullptr;
    FS::ParticleEmitter* moveSmokeEmitterPtr_ = nullptr;
    FS::ParticleEmitter* explosionEmitterPtr_ = nullptr;
    int soundHandleID_ = -1;

    FS::Vector4 spotColor_ = { 1.0f, 1.0f, 0.8f, 1.0f };
    float spotIntensity_ = 8.0f;
    float spotDistance_ = 20.0f;
    float spotAngleDeg_ = 30.0f;
    float spotVolumetric_ = 4.0f;
    FS::Vector3 spotDirection_ = { 0.0f, -1.0f, 0.0f };
};
