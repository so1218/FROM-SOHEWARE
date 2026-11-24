#pragma once
#include "Engine.h"
#include "IEnemyBehavior.h"
#include "PropertyBinder.h"
#include "AnimationModel.h"
#include "ParticleEmitter.h"
#include "BurnStatus.h"
#include <random>

namespace FS { class Terrain; }

class Player;

// ゾンビで共通の設定。出現のさせ方も持つ
struct ZombieConfig
{
    void Bind(FS::PropertyBinder& binder);
    void Draw(FS::PropertyBinder& binder);

    EnemyBodyConfig body = { 60, { 1.0f, 1.0f, 1.0f }, { 0.8f, 1.8f, 0.8f }, { 0.0f, 0.9f, 0.0f } };
    float turnSpeed = 6.0f;

    // 出現
    bool isSpawnEnabled = true;
    int maxAliveCount = 10;         
    float spawnInterval = 5.0f;     
    float spawnMinDistance = 15.0f; 
    float spawnMaxDistance = 35.0f; 
    float despawnDistance = 90.0f;  
    float emergeTime = 2.5f;        
    float emergeDepth = 1.8f;       
    float emergeBuryMargin = 0.3f;   

    // 発見
    float sightRange = 40.0f;       
    float closeNoticeRange = 4.0f;  
    float hearingRange = 60.0f;     
    float loseTime = 1.0f;          
    float searchTime = 6.0f;        

    // 移動
    float chaseSpeed = 2.2f;
    float wanderSpeed = 0.8f;
    float wanderRadius = 8.0f;
    float separationRadius = 1.2f;  

    // 攻撃
    float attackRange = 1.6f;
    int attackDamage = 10;
    float attackHitTime = 0.45f;    
    float attackReachMargin = 0.5f;     
    float attackHalfAngle = 60.0f;      
    float attackTrackEndTime = 0.3f;    

    // アニメーション速度
    float emergeAnimSpeed = 1.0f;
    float walkAnimSpeed = 1.0f;
    float attackAnimSpeed = 1.0f;
    float animBlendTime = 0.25f;

    // 撃たれたときの音
    float damagedSoundVolume = 0.6f;
    float sparkSoundVolume = 0.5f;

    // 頭の当たり判定
    float headRadius = 1.2f;
    FS::Vector3 headOffset = { 0.0f, 0.6f, 0.0f };   
    float headDamageRate = 2.0f;
    float flinchAnimSpeed = 0.5f;
    float flinchEndTime = 0.6f;    
    float headshotSoundVolume = 0.8f;

    // 燃えている草の上での継続ダメージ
    BurnConfig burn = { 0.2f, 20.0f, 0.5f, 3.0f };
    // 燃えている間に、回路が焼ける音を鳴らす間隔と大きさ
    float burnSparkInterval = 0.8f;
    float burnSparkVolume = 0.35f;
    float burnSparkHearingDistance = 30.0f; 

    // 這い出るときの音
    float emergeSoundVolume = 0.7f;
    float emergeSoundMaxDistance = 60.0f;
    // 吠える
    float roarVolume = 0.6f;
    float roarMaxDistance = 60.0f;
    float roarMinInterval = 6.0f;
    float roarMaxInterval = 14.0f;

    // 倒されて爆発したときに、足元の草に火がつく半径
    float deathIgniteRadius = 4.0f;
};

class ZombieBehavior : public IEnemyBehavior
{
public:
    // アニメーションと見た目のモデルの名前
    static constexpr const char* kMeshName = "humanMesh";
    static constexpr const char* kEmergeAnimation = "zombieEmerge";
    static constexpr const char* kIdleAnimation = "humanIdle";
    static constexpr const char* kWalkAnimation = "zombieWalk";
    static constexpr const char* kAttackAnimation = "zombieAttack";
    static constexpr const char* kDeathAnimation = "zombieDeath";
    static constexpr const char* kFlinchAnimation = "humanHit";

    // materialSource は全員で共有するマテリアルを持つモデル
    ZombieBehavior(const ZombieConfig& config, const FS::Terrain* terrain, const FS::AnimationModel* materialSource, const FS::Vector3& spawnPosition, float spawnYaw);

    void Initialize(Enemy* owner) override;
    void Update(Enemy* owner) override;
    void DebugGui(Enemy* owner) override;
    void Draw(Enemy* owner) override;

    const EnemyBodyConfig& GetBodyConfig() const override { return config_.body; }
    std::string GetDamageParticleName() const override { return "zombieHitBullet"; }
    bool DrawsBodyModel() const override { return false; }

    bool OnHealthDepleted(Enemy* owner) override;
    void OnDeath(Enemy* owner) override;
    void OnDespawn(Enemy* owner) override;
    int ModifyDamage(Enemy* owner, int damage, const FS::Collider* hitCollider) override;
    void OnTakeDamage(Enemy* owner, int damage, const FS::Vector3& hitPoint, const FS::Vector3& hitNormal) override;

    // ゾンビどうしを押し離すのに使う
    bool IsDying() const { return state_ == State::Dying; }

private:
    enum class State
    {
        Emerge, 
        Chase,  
        Attack, 
        Search, 
        Wander, 
        Dying,  
        Flinch,     
    };

    void UpdateEmerge(Enemy* owner, float deltaTime);
    void UpdateChase(Enemy* owner, float deltaTime);
    void UpdateAttack(Enemy* owner, float deltaTime);
    void UpdateSearch(Enemy* owner, float deltaTime);
    void UpdateWander(Enemy* owner, float deltaTime);
    void UpdateDying(Enemy* owner, float deltaTime);
    void UpdateFlinch(Enemy* owner, float deltaTime);
    // 頭の当たり判定を頭の骨の位置へ動かす
    void UpdateHeadCollider(Enemy* owner);
    void ChangeState(State state);

    // 見えているか、銃声が聞こえたら、プレイヤーの位置を覚えて true
    bool TryNoticePlayer(const FS::Vector3& position);
    // 地面に沿って歩き、進む向きを向く
    bool MoveToward(Enemy* owner, const FS::Vector3& target, float speed, float deltaTime);
    // 正面から左右 attackHalfAngle の中にいるか
    bool IsInAttackAngle(Enemy* owner, const FS::Vector3& targetPosition) const;
    void FaceDirection(Enemy* owner, const FS::Vector3& direction, float deltaTime);
    float GetGroundHeight(float x, float z) const;
    void PlayAnimation(const char* name, bool isLoop, float speed);
    float RandomRange(float min, float max);
    void ReleaseEffects();
    // 足元の炎で燃え、継続ダメージを受ける
    void UpdateBurning(Enemy* owner, float deltaTime);
    void UpdateBurnEffects(Enemy* owner);
    // 生きて動いている間、ときどき吠える
    void UpdateRoar(Enemy* owner, float deltaTime);
    // 本体と同じ大きさで立たせたときの、足元から頭までの高さ
    float MeasureHeadHeight();

    const ZombieConfig& config_;
    const FS::Terrain* terrain_ = nullptr;
    const FS::AnimationModel* materialSource_ = nullptr;
    Player* player_ = nullptr;
    std::unique_ptr<FS::AnimationModel> animationModel_;
    FS::Collider* headCollider_ = nullptr;
    std::string headJointName_;
    // ModifyDamage で頭に当たったかを覚え、OnTakeDamage で使う
    bool wasHeadHit_ = false;
    FS::ParticleEmitter* auraEmitterPtr_ = nullptr;
    FS::ParticleEmitter* dustEmitterPtr_ = nullptr;
    FS::ParticleEmitter* explosionEmitterPtr_ = nullptr;
    FS::ParticleEmitter* deathSparkEmitterPtr_ = nullptr;
    FS::ParticleEmitter* burnEmbersEmitterPtr_ = nullptr;
    bool isBurnEffectPlaying_ = false;
    BurnStatus burn_;
    float burnSparkTimer_ = 0.0f;
    // 次に吠えるまでの時間
    float roarTimer_ = 0.0f;
    // 這い出る前に埋まっている深さ
    float buriedDepth_ = 0.0f;

    State state_ = State::Emerge;
    float stateTimer_ = 0.0f;
    float lostTimer_ = 0.0f;
    bool hasHitThisAttack_ = false;
    int lastPlayerShotCount_ = 0;

    FS::Vector3 spawnPosition_;
    float spawnYaw_ = 0.0f;
    FS::Vector3 lastKnownPlayerPosition_ = { 0.0f, 0.0f, 0.0f };
    FS::Vector3 wanderTarget_ = { 0.0f, 0.0f, 0.0f };

    std::mt19937 random_;
};
