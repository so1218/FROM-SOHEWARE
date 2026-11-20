#include "pch.h"
#include "ZombieBehavior.h"
#include "Enemy.h"
#include "Player.h"
#include "GameDefine.h"
#include "GameObjectManager.h"
#include "TimeManager.h"
#include "AnimationManager.h"
#include "MathUtils.h"
#include "Terrain.h"

using namespace FS;

namespace
{

// 目的地に着いたとみなす距離
constexpr float kArriveDistance = 0.3f;
// うろついて目的地に着いたら、ここまで立ち止まる
constexpr float kWanderPauseTime = 3.0f;
// 殴る範囲から少し出ても、振り下ろしは当たる
constexpr float kAttackReachMargin = 0.5f;
// 攻撃や倒れるアニメーションが無いときに次へ進むまで
constexpr float kAnimationFallbackTime = 3.0f;

bool HasAnimation(const char* name)
{
    return AnimationManager::GetInstance()->Get(name) != nullptr;
}

float HorizontalDistance(const Vector3& from, const Vector3& to)
{
    const float x = to.x - from.x;
    const float z = to.z - from.z;
    return std::sqrt(x * x + z * z);
}

}

ZombieBehavior::ZombieBehavior(const ZombieConfig& config, const Terrain* terrain, const Vector3& spawnPosition, float spawnYaw)
    : config_(config), terrain_(terrain), spawnPosition_(spawnPosition), spawnYaw_(spawnYaw), random_(std::random_device{}())
{
}

void ZombieBehavior::Initialize(Enemy* owner)
{
    const char* firstAnimation = HasAnimation(kEmergeAnimation) ? kEmergeAnimation : kWalkAnimation;
    animationModel_ = std::make_unique<AnimationModel>(owner->GetEngine(), kMeshName, firstAnimation);

    // 体の形に沿って出し、動きについてくる
    auraEmitterPtr_ = owner->GetEngine()->GetParticleSystem()->SpawnEmitter("zombieAura");
    if (auraEmitterPtr_)
    {
        auraEmitterPtr_->SetTargetAnimationModel(animationModel_.get());
        auraEmitterPtr_->Play();
    }

    // 地面の下に埋まった所から始める
    WorldTransform& transform = owner->GetTransform();
    transform.translation_ = { spawnPosition_.x, GetGroundHeight(spawnPosition_.x, spawnPosition_.z) - config_.emergeDepth, spawnPosition_.z };
    transform.SetRotation(Quaternion::QuaternionFromEuler({ 0.0f, spawnYaw_, 0.0f }));
    owner->SyncTransform();

    PlayAnimation(kEmergeAnimation, false, config_.emergeAnimSpeed);
    ChangeState(State::Emerge);
}

void ZombieBehavior::Update(Enemy* owner)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    stateTimer_ += deltaTime;

    if (!player_)
    {
        player_ = owner->GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
        // 出てくる前に撃った分は銃声として扱わない
        lastPlayerShotCount_ = player_ ? player_->GetShotCount() : 0;
    }

    switch (state_)
    {
    case State::Emerge: UpdateEmerge(owner, deltaTime); break;
    case State::Chase:  UpdateChase(owner, deltaTime);  break;
    case State::Attack: UpdateAttack(owner, deltaTime); break;
    case State::Search: UpdateSearch(owner, deltaTime); break;
    case State::Wander: UpdateWander(owner, deltaTime); break;
    case State::Dying:  UpdateDying(owner, deltaTime);  break;
    }

    // Dying の中で Die() されたときはここで終わる
    if (!owner->IsActive())
    {
        return;
    }

    owner->SyncTransform();

    // 見た目は本体の位置と向きに合わせる
    const WorldTransform& body = owner->GetTransform();
    WorldTransform& visual = animationModel_->GetTransform();
    visual.translation_ = body.translation_;
    visual.rotation_ = body.rotation_;
    visual.scale_ = config_.body.scale;
    animationModel_->Update();
}

void ZombieBehavior::UpdateEmerge(Enemy* owner, float deltaTime)
{
    // 最初は勢いよく、地面に出る直前はゆっくり上がる
    const float progress = std::clamp(stateTimer_ / std::max(config_.emergeTime, 0.01f), 0.0f, 1.0f);
    const float eased = 1.0f - (1.0f - progress) * (1.0f - progress);

    Vector3& position = owner->GetTransform().translation_;
    position.y = GetGroundHeight(position.x, position.z) - config_.emergeDepth * (1.0f - eased);

    if (progress < 1.0f)
    {
        return;
    }

    // 這い出たら、見えていれば追いかけ、隠れていればうろつく
    if (TryNoticePlayer(position))
    {
        ChangeState(State::Chase);
    }
    else
    {
        wanderTarget_ = position;
        ChangeState(State::Wander);
    }
}

void ZombieBehavior::UpdateChase(Enemy* owner, float deltaTime)
{
    const Vector3& position = owner->GetTransform().translation_;

    // 少しの間見えなくても、すぐには見失わない
    const bool canSee = TryNoticePlayer(position);
    lostTimer_ = canSee ? 0.0f : lostTimer_ + deltaTime;
    if (lostTimer_ >= config_.loseTime)
    {
        ChangeState(State::Search);
        return;
    }

    if (canSee && HorizontalDistance(position, player_->GetTransform().translation_) <= config_.attackRange)
    {
        ChangeState(State::Attack);
        return;
    }

    PlayAnimation(kWalkAnimation, true, config_.walkAnimSpeed);
    MoveToward(owner, lastKnownPlayerPosition_, config_.chaseSpeed, deltaTime);
}

void ZombieBehavior::UpdateAttack(Enemy* owner, float deltaTime)
{
    PlayAnimation(kAttackAnimation, false, config_.attackAnimSpeed);

    const Vector3& position = owner->GetTransform().translation_;
    const Vector3 playerPosition = player_->GetTransform().translation_;
    FaceDirection(owner, playerPosition - position, deltaTime);

    // 振り下ろした瞬間に、まだ届く所にいれば当たる
    const bool hasAttackAnimation = HasAnimation(kAttackAnimation);
    const float attackProgress = hasAttackAnimation ? animationModel_->GetNormalizedTime() : stateTimer_ / kAnimationFallbackTime;
    if (!hasHitThisAttack_ && attackProgress >= config_.attackHitTime)
    {
        hasHitThisAttack_ = true;
        if (HorizontalDistance(position, playerPosition) <= config_.attackRange + kAttackReachMargin)
        {
            player_->TakeDamage(config_.attackDamage);
        }
    }

    const bool isFinished = hasAttackAnimation ? animationModel_->IsFinished() : stateTimer_ >= kAnimationFallbackTime;
    if (isFinished || stateTimer_ >= kAnimationFallbackTime)
    {
        ChangeState(State::Chase);
    }
}

void ZombieBehavior::UpdateSearch(Enemy* owner, float deltaTime)
{
    const Vector3& position = owner->GetTransform().translation_;
    if (TryNoticePlayer(position))
    {
        ChangeState(State::Chase);
        return;
    }

    // 最後に見た場所まで行き、そこで探す
    if (MoveToward(owner, lastKnownPlayerPosition_, config_.chaseSpeed * 0.7f, deltaTime))
    {
        PlayAnimation(kIdleAnimation, true, 1.0f);
    }
    else
    {
        PlayAnimation(kWalkAnimation, true, config_.walkAnimSpeed);
    }

    if (stateTimer_ >= config_.searchTime)
    {
        wanderTarget_ = position;
        ChangeState(State::Wander);
    }
}

void ZombieBehavior::UpdateWander(Enemy* owner, float deltaTime)
{
    const Vector3& position = owner->GetTransform().translation_;
    if (TryNoticePlayer(position))
    {
        ChangeState(State::Chase);
        return;
    }

    if (!MoveToward(owner, wanderTarget_, config_.wanderSpeed, deltaTime))
    {
        PlayAnimation(kWalkAnimation, true, config_.walkAnimSpeed * 0.6f);
        return;
    }

    // 着いたら少し立ち止まってから、近くの別の場所へ向かう
    PlayAnimation(kIdleAnimation, true, 1.0f);
    if (stateTimer_ >= kWanderPauseTime)
    {
        const float angle = RandomRange(0.0f, Math::PI * 2.0f);
        const float distance = RandomRange(config_.wanderRadius * 0.3f, config_.wanderRadius);
        wanderTarget_ = { position.x + std::cos(angle) * distance, position.y, position.z + std::sin(angle) * distance };
        stateTimer_ = 0.0f;
    }
}

void ZombieBehavior::UpdateDying(Enemy* owner, float deltaTime)
{
    PlayAnimation(kDeathAnimation, false, 1.0f);

    const bool isFinished = HasAnimation(kDeathAnimation) ? animationModel_->IsFinished() : true;
    if (isFinished || stateTimer_ >= kAnimationFallbackTime)
    {
        owner->Die();
    }
}

void ZombieBehavior::ChangeState(State state)
{
    state_ = state;
    stateTimer_ = 0.0f;
    lostTimer_ = 0.0f;
    hasHitThisAttack_ = false;
}

bool ZombieBehavior::TryNoticePlayer(const Vector3& position)
{
    if (!player_ || !player_->IsControllable())
    {
        return false;
    }

    const Vector3 playerPosition = player_->GetTransform().translation_;
    const float distance = (playerPosition - position).Length();

    // 銃声は隠れていても聞こえる
    const int playerShotCount = player_->GetShotCount();
    const bool heardShot = playerShotCount != lastPlayerShotCount_ && distance <= config_.hearingRange;
    lastPlayerShotCount_ = playerShotCount;

    // 霧や草に隠れていると、すぐそばまで来ないと気づかない
    const float noticeRange = player_->IsHidden() ? config_.closeNoticeRange : config_.sightRange;
    if (distance <= noticeRange || heardShot)
    {
        lastKnownPlayerPosition_ = playerPosition;
        return true;
    }
    return false;
}

bool ZombieBehavior::MoveToward(Enemy* owner, const Vector3& target, float speed, float deltaTime)
{
    Vector3& position = owner->GetTransform().translation_;
    const Vector3 toTarget = { target.x - position.x, 0.0f, target.z - position.z };
    const float distance = toTarget.Length();

    if (distance > kArriveDistance)
    {
        const Vector3 direction = toTarget * (1.0f / distance);
        const float step = std::min(speed * deltaTime, distance);
        position.x += direction.x * step;
        position.z += direction.z * step;
        FaceDirection(owner, direction, deltaTime);
    }
    position.y = GetGroundHeight(position.x, position.z);

    return distance <= kArriveDistance;
}

void ZombieBehavior::FaceDirection(Enemy* owner, const Vector3& direction, float deltaTime)
{
    if (direction.x * direction.x + direction.z * direction.z < 0.0001f)
    {
        return;
    }

    // モデルは +Z が正面
    WorldTransform& transform = owner->GetTransform();
    const float targetYaw = std::atan2(direction.x, direction.z);
    const Quaternion targetRotation = Quaternion::QuaternionFromEuler({ 0.0f, targetYaw, 0.0f });
    transform.SetRotation(Quaternion::Slerp(transform.rotation_, targetRotation, std::min(config_.turnSpeed * deltaTime, 1.0f)));
}

float ZombieBehavior::GetGroundHeight(float x, float z) const
{
    return terrain_ ? terrain_->GetHeight(x, z) : 0.0f;
}

void ZombieBehavior::PlayAnimation(const char* name, bool isLoop, float speed)
{
    // 用意していないアニメーションは、歩きで代わりにする
    const char* animation = HasAnimation(name) ? name : kWalkAnimation;
    animationModel_->Play(animation, isLoop, speed, config_.animBlendTime);
}

float ZombieBehavior::RandomRange(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(random_);
}

void ZombieBehavior::OnDeath(Enemy* owner)
{
    ReleaseAura();
}

void ZombieBehavior::OnDespawn(Enemy* owner)
{
    ReleaseAura();
}

void ZombieBehavior::ReleaseAura()
{
    if (auraEmitterPtr_)
    {
        auraEmitterPtr_->Stop();
        auraEmitterPtr_->Destroy();
        auraEmitterPtr_ = nullptr;
    }
}

bool ZombieBehavior::OnHealthDepleted(Enemy* owner)
{
    if (state_ != State::Dying)
    {
        ChangeState(State::Dying);
    }
    return true;
}

int ZombieBehavior::ModifyDamage(Enemy* owner, int damage, const Collider* hitCollider)
{
    if (state_ == State::Dying)
    {
        return 0;
    }

    // 撃たれたら、隠れていてもそちらへ向かう
    if (player_ && (state_ == State::Search || state_ == State::Wander))
    {
        lastKnownPlayerPosition_ = player_->GetTransform().translation_;
        ChangeState(State::Chase);
    }
    return damage;
}

void ZombieBehavior::Draw(Enemy* owner)
{
    animationModel_->Draw();
}

void ZombieBehavior::DebugGui(Enemy* owner)
{
    constexpr const char* kStateNames[] = { "這い出る", "追跡", "攻撃", "捜索", "うろつく", "倒れる" };
    ImGui::Text("状態: %s", kStateNames[static_cast<int>(state_)]);
}

void ZombieConfig::Bind(PropertyBinder& binder)
{
    binder.Bind("HP", &body.maxHP, 60, 1.0f, 1, 100000);
    binder.Bind("Scale", &body.scale, { 1.0f, 1.0f, 1.0f });
    binder.Bind("ColliderSize", &body.colliderSize, { 0.8f, 1.8f, 0.8f });
    binder.Bind("ColliderOffset", &body.colliderOffset, { 0.0f, 0.9f, 0.0f });
    binder.Bind("TurnSpeed", &turnSpeed, 6.0f, 0.1f, 0.0f, 50.0f);

    binder.Bind("SpawnEnable", &isSpawnEnabled, true);
    binder.Bind("MaxAliveCount", &maxAliveCount, 10, 1.0f, 0, 50);
    binder.Bind("SpawnInterval", &spawnInterval, 5.0f, 0.1f, 0.1f, 120.0f);
    binder.Bind("SpawnMinDistance", &spawnMinDistance, 15.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind("SpawnMaxDistance", &spawnMaxDistance, 35.0f, 0.5f, 1.0f, 300.0f);
    binder.Bind("DespawnDistance", &despawnDistance, 90.0f, 1.0f, 10.0f, 1000.0f);
    binder.Bind("EmergeTime", &emergeTime, 2.5f, 0.05f, 0.1f, 10.0f);
    binder.Bind("EmergeDepth", &emergeDepth, 1.8f, 0.05f, 0.0f, 5.0f);

    binder.Bind("SightRange", &sightRange, 40.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind("CloseNoticeRange", &closeNoticeRange, 4.0f, 0.1f, 0.0f, 50.0f);
    binder.Bind("HearingRange", &hearingRange, 60.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind("LoseTime", &loseTime, 1.0f, 0.05f, 0.0f, 10.0f);
    binder.Bind("SearchTime", &searchTime, 6.0f, 0.1f, 0.0f, 60.0f);

    binder.Bind("ChaseSpeed", &chaseSpeed, 2.2f, 0.05f, 0.0f, 20.0f);
    binder.Bind("WanderSpeed", &wanderSpeed, 0.8f, 0.05f, 0.0f, 20.0f);
    binder.Bind("WanderRadius", &wanderRadius, 8.0f, 0.1f, 0.0f, 100.0f);
    binder.Bind("SeparationRadius", &separationRadius, 1.2f, 0.05f, 0.0f, 10.0f);

    binder.Bind("AttackRange", &attackRange, 1.6f, 0.05f, 0.1f, 10.0f);
    binder.Bind("AttackDamage", &attackDamage, 10, 1.0f, 0, 1000);
    binder.Bind("AttackHitTime", &attackHitTime, 0.45f, 0.01f, 0.0f, 1.0f);

    binder.Bind("EmergeAnimSpeed", &emergeAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("WalkAnimSpeed", &walkAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("AttackAnimSpeed", &attackAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("AnimBlendTime", &animBlendTime, 0.25f, 0.01f, 0.0f, 2.0f);
}

void ZombieConfig::Draw(PropertyBinder& binder)
{
    if (ImGui::TreeNode("本体"))
    {
        binder.Draw("HP", "HP");
        binder.Draw("Scale", "スケール");
        binder.Draw("ColliderSize", "当たり判定の大きさ");
        binder.Draw("ColliderOffset", "当たり判定の位置");
        binder.Draw("TurnSpeed", "向きを変える速さ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("出現"))
    {
        binder.Draw("SpawnEnable", "出す");
        binder.Draw("MaxAliveCount", "同時に出ている数の上限");
        binder.Draw("SpawnInterval", "出す間隔 (秒)");
        binder.Draw("SpawnMinDistance", "プレイヤーから一番近くて");
        binder.Draw("SpawnMaxDistance", "プレイヤーから一番遠くて");
        binder.Draw("DespawnDistance", "これより離れたら消す");
        binder.Draw("EmergeTime", "這い出るのにかかる秒数");
        binder.Draw("EmergeDepth", "埋まっている深さ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("発見"))
    {
        binder.Draw("SightRange", "見えているプレイヤーに気づく距離");
        binder.Draw("CloseNoticeRange", "隠れていても気づく距離");
        binder.Draw("HearingRange", "銃声に気づく距離");
        binder.Draw("LoseTime", "見えなくなってから見失うまで (秒)");
        binder.Draw("SearchTime", "最後に見た場所を探す秒数");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("移動"))
    {
        binder.Draw("ChaseSpeed", "追いかける速さ (m/秒)");
        binder.Draw("WanderSpeed", "うろつく速さ (m/秒)");
        binder.Draw("WanderRadius", "うろつく範囲");
        binder.Draw("SeparationRadius", "ゾンビどうしの間隔");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("攻撃"))
    {
        binder.Draw("AttackRange", "殴る距離");
        binder.Draw("AttackDamage", "ダメージ");
        binder.Draw("AttackHitTime", "当たる瞬間 (アニメーションの 0 ~ 1)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("アニメーション"))
    {
        binder.Draw("EmergeAnimSpeed", "這い出る速さ");
        binder.Draw("WalkAnimSpeed", "歩く速さ");
        binder.Draw("AttackAnimSpeed", "殴る速さ");
        binder.Draw("AnimBlendTime", "切り替えの補間時間 (秒)");
        ImGui::TreePop();
    }
}
