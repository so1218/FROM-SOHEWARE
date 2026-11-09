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
    float turnSpeed = 6.0f;       

    // 発見
    float sightRange = 60.0f;     
    float hearingRange = 120.0f;  
    float loseRange = 150.0f;     

    // 追跡
    float moveSpeed = 18.0f;
    float acceleration = 3.0f;    
    float keepDistance = 25.0f;   
    float keepHeight = 10.0f;     
    float orbitSpeed = 0.6f;      
    float wobbleAmount = 2.0f;    

    // 攻撃
    float attackInterval = 2.5f;  
    float lockOnTime = 1.0f;      
    int burstCount = 6;           
    float shotInterval = 0.1f;    
    int damagePerShot = 4;
    float shotSpread = 0.03f;     
    float maxFireDistance = 80.0f;
    FS::Vector3 muzzleOffset = { 0.0f, 2.0f, 0.0f };

    // スラスター 
    FS::Vector3 thrusterOffset = { 1.0f, 1.0f, 1.0f };  
    FS::Vector3 thrusterScale = { 0.5f, 0.5f, 0.5f };
    FS::Vector3 thrusterColliderSize = { 1.0f, 1.0f, 1.0f };
    int thrusterMaxHP = 30;
    float propellerSpeed = 40.0f;  
    float bodyDamageRate = 0.3f;   
    int thrustersToCrash = 2;      

    // 墜落
    float crashTime = 5.0f;        
    float crashGravity = 30.0f;

    // スポットライト
    FS::Vector4 spotColor = { 0.076f, 0.216f, 1.0f, 1.0f };
    float spotIntensity = 30.85f;
    float spotDistance = 28.24f;
    float spotAngleDeg = 28.13f;
    float spotVolumetric = 200.0f;
    FS::Vector3 spotLocalDirection = { 0.0f, -0.3f, -1.0f };   
};

// 空中を飛ぶドローン型の敵
class FloatingBehavior : public IEnemyBehavior
{
public:
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
    const FS::Terrain* terrain_ = nullptr;  
    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;

    // 状態
    State state_ = State::Patrol;
    float stateTimer_ = 0.0f;        
    float elapsedTime_ = 0.0f;
    int shotsFired_ = 0;             
    int lastPlayerShotCount_ = 0;    
    std::mt19937 random_;

    // 移動
    FS::Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    FS::Vector3 aimDirection_ = { 0.0f, 0.0f, 1.0f };  
    float orbitAngle_ = 0.0f;
    float orbitDirection_ = 1.0f;    
    float orbitChangeTimer_ = 0.0f;  

    // スラスター
    int thrusterHP_[kThrusterCount] = {};
    FS::Collider* thrusterColliders_[kThrusterCount] = {};  
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

    // 巡回
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
