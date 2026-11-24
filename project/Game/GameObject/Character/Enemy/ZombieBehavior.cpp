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
#include "AudioPlayer.h"
#include "RendererManager.h"
#include "CollisionConfig.h"

using namespace FS;

namespace
{

// 目的地に着いたとみなす距離
constexpr float kArriveDistance = 0.3f;
// うろついて目的地に着いたら、ここまで立ち止まる
constexpr float kWanderPauseTime = 3.0f;
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

ZombieBehavior::ZombieBehavior(const ZombieConfig& config, const Terrain* terrain, const AnimationModel* materialSource, const Vector3& spawnPosition, float spawnYaw)
    : config_(config), terrain_(terrain), materialSource_(materialSource), spawnPosition_(spawnPosition), spawnYaw_(spawnYaw), random_(std::random_device{}())
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

    // 頭は弾のレイだけが当たる球にする
    headCollider_ = owner->AddSphereCollider(config_.headRadius);
    headCollider_->SetCollisionLayer(CollisionLayer::EnemyWeakPoint);
    headCollider_->SetRaycastOnly(true);
    headCollider_->SetRotateCenterOffset(false);
    headCollider_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });   // 体と見分けやすいよう赤で描く

    // Mixamo のモデルは、書き出し方で骨の名前の頭が変わる
    for (const char* jointName : { "mixamorig1:Head", "mixamorig:Head" })
    {
        if (animationModel_->GetSkeleton().jointMap.contains(jointName))
        {
            headJointName_ = jointName;
            break;
        }
    }

    // 頭のてっぺんまで地面の下に埋まった所から始める
    const float headHeight = MeasureHeadHeight();
    buriedDepth_ = (headHeight > 0.1f) ? headHeight + config_.emergeBuryMargin * config_.body.scale.y : config_.emergeDepth;

    const float groundHeight = GetGroundHeight(spawnPosition_.x, spawnPosition_.z);
    WorldTransform& transform = owner->GetTransform();
    transform.translation_ = { spawnPosition_.x, groundHeight - buriedDepth_, spawnPosition_.z };
    transform.SetRotation(Quaternion::QuaternionFromEuler({ 0.0f, spawnYaw_, 0.0f }));
    owner->SyncTransform();

    // 這い出ている間、足元から土煙を上げる
    dustEmitterPtr_ = owner->GetEngine()->GetParticleSystem()->SpawnEmitter("zombieEmergeDust");
    if (dustEmitterPtr_)
    {
        dustEmitterPtr_->SetPosition({ spawnPosition_.x, groundHeight, spawnPosition_.z });
        dustEmitterPtr_->Play();
    }

    explosionEmitterPtr_ = owner->GetEngine()->GetParticleSystem()->SpawnEmitter("floatingEnemyExplosion");
    deathSparkEmitterPtr_ = owner->GetEngine()->GetParticleSystem()->SpawnEmitter("enemyDeathSpark");

    PlayAnimation(kEmergeAnimation, false, config_.emergeAnimSpeed);
    ChangeState(State::Emerge);

    AudioPlayer::GetInstance().Play3D("zombieEmerge", spawnPosition_,
        { .volume = config_.emergeSoundVolume, .maxDistance = config_.emergeSoundMaxDistance });
    // 出てきてすぐ吠えないよう、最初も間を空ける
    roarTimer_ = RandomRange(config_.roarMinInterval, std::max(config_.roarMinInterval, config_.roarMaxInterval));
}

float ZombieBehavior::MeasureHeadHeight()
{
    // Mixamo のモデルは、書き出し方で骨の名前の頭が変わる
    constexpr const char* kHeadJointNames[] = { "mixamorig1:HeadTop_End", "mixamorig:HeadTop_End", "mixamorig1:Head", "mixamorig:Head" };
    const auto& jointMap = animationModel_->GetSkeleton().jointMap;

    for (const char* jointName : kHeadJointNames)
    {
        if (!jointMap.contains(jointName))
        {
            continue;
        }

        // 原点に本体と同じ大きさで置き、姿勢を一度計算してから測る
        WorldTransform& visual = animationModel_->GetTransform();
        visual.translation_ = { 0.0f, 0.0f, 0.0f };
        visual.scale_ = config_.body.scale;
        animationModel_->Update();
        return animationModel_->GetJointWorldMatrix(jointName).m[3][1];
    }
    return 0.0f;
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
    case State::Flinch: UpdateFlinch(owner, deltaTime); break;
    }

    // Dying の中で Die() されたときはここで終わる
    if (!owner->IsActive())
    {
        return;
    }

    // 燃えて倒れたときもここで終わる
    UpdateBurning(owner, deltaTime);
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

    // テクスチャを変えたときも反映されるよう、毎フレーム写す
    animationModel_->ShareMaterialsFrom(materialSource_);
    animationModel_->Update();
    UpdateHeadCollider(owner);

    UpdateRoar(owner, deltaTime);
}

void ZombieBehavior::UpdateEmerge(Enemy* owner, float deltaTime)
{
    // 土をかき分けるように、出始めと出きる直前はゆっくり上がる
    const float progress = std::clamp(stateTimer_ / std::max(config_.emergeTime, 0.01f), 0.0f, 1.0f);
    const float eased = progress * progress * (3.0f - 2.0f * progress);

    Vector3& position = owner->GetTransform().translation_;
    position.y = GetGroundHeight(position.x, position.z) - buriedDepth_ * (1.0f - eased);

    if (progress < 1.0f)
    {
        return;
    }

    // 出きったら土煙を止める
    if (dustEmitterPtr_)
    {
        dustEmitterPtr_->Stop();
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
    const bool hasAttackAnimation = HasAnimation(kAttackAnimation);
    const float attackProgress = hasAttackAnimation ? animationModel_->GetNormalizedTime() : stateTimer_ / kAnimationFallbackTime;

    // 振りかぶる間だけプレイヤーへ向き直る
    if (attackProgress < config_.attackTrackEndTime)
    {
        FaceDirection(owner, playerPosition - position, deltaTime);
    }

    // 振り下ろした瞬間に、正面の届く所にいれば当たる
    if (!hasHitThisAttack_ && attackProgress >= config_.attackHitTime)
    {
        hasHitThisAttack_ = true;
        if (HorizontalDistance(position, playerPosition) <= config_.attackRange + config_.attackReachMargin &&
            IsInAttackAngle(owner, playerPosition))
        {
            // ゾンビから離れる向きへ押し飛ばす
            Vector3 knockbackDirection = { playerPosition.x - position.x, 0.0f, playerPosition.z - position.z };
            if (knockbackDirection.LengthSq() < 0.0001f)
            {
                knockbackDirection = owner->GetTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f });
            }
            player_->TakeHit(config_.attackDamage, knockbackDirection);
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

void ZombieBehavior::UpdateFlinch(Enemy* owner, float deltaTime)
{
    // ひるむアニメーションが無いときは、その場で立ち止まる
    const bool hasFlinchAnimation = HasAnimation(kFlinchAnimation);
    PlayAnimation(hasFlinchAnimation ? kFlinchAnimation : kIdleAnimation, false, config_.flinchAnimSpeed);

    // 一番のけぞった所まで流したら、止めずにそのまま追いかけに戻す
    constexpr float kFlinchFallbackTime = 0.5f;
    const bool isFinished = hasFlinchAnimation
        ? animationModel_->IsFinished() || animationModel_->GetNormalizedTime() >= config_.flinchEndTime
        : stateTimer_ >= kFlinchFallbackTime;
    if (isFinished || stateTimer_ >= kAnimationFallbackTime)
    {
        if (player_)
        {
            lastKnownPlayerPosition_ = player_->GetTransform().translation_;
        }
        ChangeState(State::Chase);
    }
}

void ZombieBehavior::UpdateHeadCollider(Enemy* owner)
{
    if (!headCollider_)
    {
        return;
    }

    const bool isEnabled = !headJointName_.empty() && state_ != State::Dying;
    headCollider_->SetEnable(isEnabled);
    if (!isEnabled)
    {
        return;
    }

    const Matrix4x4 headMatrix = animationModel_->GetJointWorldMatrix(headJointName_);
    const Vector3 headPosition = Vector3(headMatrix.m[3][0], headMatrix.m[3][1], headMatrix.m[3][2]) + config_.headOffset;
    headCollider_->SetCenterOffset(headPosition - owner->GetTransform().GetWorldPosition());
    headCollider_->SetRadius(config_.headRadius);
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
    if (!player_ || !player_->CanBeNoticed())
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

bool ZombieBehavior::IsInAttackAngle(Enemy* owner, const Vector3& targetPosition) const
{
    const Vector3& position = owner->GetTransform().translation_;
    const float toTargetX = targetPosition.x - position.x;
    const float toTargetZ = targetPosition.z - position.z;
    const float distance = std::sqrt(toTargetX * toTargetX + toTargetZ * toTargetZ);
    // 重なるほど近いときは向きが決まらないので当てる
    if (distance < 0.001f)
    {
        return true;
    }

    // モデルは +Z が正面
    const Vector3 forward = owner->GetTransform().rotation_.RotateVector({ 0.0f, 0.0f, 1.0f });
    const float forwardLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    if (forwardLength < 0.001f)
    {
        return true;
    }

    const float cosAngle = (forward.x * toTargetX + forward.z * toTargetZ) / (forwardLength * distance);
    return cosAngle >= std::cos(config_.attackHalfAngle * Math::PI / 180.0f);
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

void ZombieBehavior::UpdateRoar(Enemy* owner, float deltaTime)
{
    // 這い出ている間と倒れていく間は吠えない
    if (state_ == State::Emerge || state_ == State::Dying)
    {
        return;
    }

    roarTimer_ -= deltaTime;
    if (roarTimer_ > 0.0f)
    {
        return;
    }
    roarTimer_ = RandomRange(config_.roarMinInterval, std::max(config_.roarMinInterval, config_.roarMaxInterval));

    AudioPlayer::GetInstance().Play3D("zombieRoar", owner->GetTransform().translation_,
        { .volume = config_.roarVolume, .maxDistance = config_.roarMaxDistance });
}

float ZombieBehavior::RandomRange(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(random_);
}

void ZombieBehavior::OnDeath(Enemy* owner)
{
    ReleaseEffects();

    // 倒れきって消える瞬間に爆発させ、急に消えるのを隠す
    AudioPlayer::GetInstance().Play("floatingEnemyExplosion", 0.4f);
    if (explosionEmitterPtr_)
    {
        explosionEmitterPtr_->SetPosition(owner->GetTransform().translation_);
        explosionEmitterPtr_->Play();
    }

    // 爆発と一緒に火花を降らせる
    if (deathSparkEmitterPtr_)
    {
        deathSparkEmitterPtr_->SetPosition(owner->GetTransform().translation_);
        deathSparkEmitterPtr_->Play();
    }

    // 足元の草に火がつく
    if (config_.deathIgniteRadius > 0.0f)
    {
        const Vector3& position = owner->GetTransform().translation_;
        owner->GetEngine()->GetRendererManager()->SubmitGrassIgnition(
            { position.x, GetGroundHeight(position.x, position.z), position.z }, config_.deathIgniteRadius);
    }
}

void ZombieBehavior::OnDespawn(Enemy* owner)
{
    ReleaseEffects();

    if (explosionEmitterPtr_)
    {
        explosionEmitterPtr_->Destroy();
        explosionEmitterPtr_ = nullptr;
    }

    if (deathSparkEmitterPtr_)
    {
        deathSparkEmitterPtr_->Destroy();
        deathSparkEmitterPtr_ = nullptr;
    }
}

void ZombieBehavior::UpdateBurning(Enemy* owner, float deltaTime)
{
    // 倒れている最中は、もう燃えても HP は減らさない。まとっている炎はそのまま残す
    if (state_ != State::Dying)
    {
        const int damage = burn_.Update(owner->GetEngine()->GetRendererManager(), owner->GetTransform().translation_, config_.burn, deltaTime);
        if (damage > 0)
        {
            owner->TakeDamageOverTime(damage);
        }
    }
    UpdateBurnEffects(owner);

    if (!burn_.IsBurning())
    {
        // ゾンビごとにずらしておき、何体も同時に燃え始めても音がそろわないようにする
        burnSparkTimer_ = RandomRange(0.0f, config_.burnSparkInterval);
        return;
    }

    // 回路が焼ける音を、間を置いて鳴らす。遠いゾンビほど小さく、遠すぎれば鳴らさない
    burnSparkTimer_ -= deltaTime;
    if (burnSparkTimer_ > 0.0f)
    {
        return;
    }
    burnSparkTimer_ = config_.burnSparkInterval * RandomRange(0.7f, 1.3f);

    if (!player_ || config_.burnSparkHearingDistance <= 0.0f)
    {
        return;
    }
    const Vector3 toPlayer = player_->GetTransform().translation_ - owner->GetTransform().translation_;
    const float distance = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
    if (distance < config_.burnSparkHearingDistance)
    {
        AudioPlayer::GetInstance().Play("zombieSpark", config_.burnSparkVolume * (1.0f - distance / config_.burnSparkHearingDistance));
    }
}

void ZombieBehavior::UpdateBurnEffects(Enemy* owner)
{
    const bool shouldPlay = burn_.IsBurning();
    if (shouldPlay && !burnEmbersEmitterPtr_)
    {
        // 見た目はプレイヤーが燃えているときと同じ
        burnEmbersEmitterPtr_ = owner->GetEngine()->GetParticleSystem()->SpawnEmitter("playerBurnEmbers");
        if (burnEmbersEmitterPtr_)
        {
            burnEmbersEmitterPtr_->SetTargetAnimationModel(animationModel_.get());
        }
    }
    if (shouldPlay == isBurnEffectPlaying_)
    {
        return;
    }

    if (burnEmbersEmitterPtr_)
    {
        if (shouldPlay)
        {
            burnEmbersEmitterPtr_->Play();
        }
        else
        {
            burnEmbersEmitterPtr_->Stop();
        }
    }
    isBurnEffectPlaying_ = shouldPlay;
}

void ZombieBehavior::ReleaseEffects()
{
    isBurnEffectPlaying_ = false;
    for (FS::ParticleEmitter** emitter : { &auraEmitterPtr_, &dustEmitterPtr_, &burnEmbersEmitterPtr_ })
    {
        if (*emitter)
        {
            (*emitter)->Stop();
            (*emitter)->Destroy();
            *emitter = nullptr;
        }
    }
}

bool ZombieBehavior::OnHealthDepleted(Enemy* owner)
{
    if (state_ != State::Dying)
    {
        ChangeState(State::Dying);
    }

    // 倒れていく間はオーラを出さない
    if (auraEmitterPtr_)
    {
        auraEmitterPtr_->Stop();
        auraEmitterPtr_->Destroy();
        auraEmitterPtr_ = nullptr;
    }
    return true;
}

int ZombieBehavior::ModifyDamage(Enemy* owner, int damage, const Collider* hitCollider)
{
    if (state_ == State::Dying)
    {
        return 0;
    }

    wasHeadHit_ = hitCollider && hitCollider == headCollider_;
    if (wasHeadHit_)
    {
        damage = static_cast<int>(static_cast<float>(damage) * config_.headDamageRate);
    }

    // 撃たれたら、隠れていてもそちらへ向かう
    if (player_ && player_->CanBeNoticed() && (state_ == State::Search || state_ == State::Wander))
    {
        lastKnownPlayerPosition_ = player_->GetTransform().translation_;
        ChangeState(State::Chase);
    }
    return damage;
}

void ZombieBehavior::OnTakeDamage(Enemy* owner, int damage, const Vector3& hitPoint, const Vector3& hitNormal)
{
    // 倒れている最中は弾が効かないので、音も鳴らさない
    if (damage <= 0)
    {
        return;
    }

    // 撃った手応えとして、距離に関係なく同じ大きさで鳴らす
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    audioPlayer.Play("zombieDamaged", config_.damagedSoundVolume);
    audioPlayer.Play("zombieSpark", config_.sparkSoundVolume);

    if (!wasHeadHit_)
    {
        return;
    }
    wasHeadHit_ = false;

    if (audioPlayer.HasSound("zombieHeadshot"))
    {
        audioPlayer.Play("zombieHeadshot", config_.headshotSoundVolume);
    }

    // 這い出ている途中はひるませない。倒れるときは、このあとで倒れる方に切り替わる
    if (state_ != State::Emerge)
    {
        ChangeState(State::Flinch);
    }
}

void ZombieBehavior::Draw(Enemy* owner)
{
    animationModel_->Draw();
}

void ZombieBehavior::DebugGui(Enemy* owner)
{
    constexpr const char* kStateNames[] = { "這い出る", "追跡", "攻撃", "捜索", "うろつく", "倒れる", "ひるむ" };
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
    binder.Bind("EmergeBuryMargin", &emergeBuryMargin, 0.3f, 0.01f, 0.0f, 3.0f);

    binder.Bind("SightRange", &sightRange, 40.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind("CloseNoticeRange", &closeNoticeRange, 4.0f, 0.1f, 0.0f, 50.0f);
    binder.Bind("HearingRange", &hearingRange, 60.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind("LoseTime", &loseTime, 1.0f, 0.05f, 0.0f, 10.0f);
    binder.Bind("SearchTime", &searchTime, 6.0f, 0.1f, 0.0f, 60.0f);

    binder.Bind("ChaseSpeed", &chaseSpeed, 2.2f, 0.05f, 0.0f, 20.0f);
    binder.Bind("WanderSpeed", &wanderSpeed, 0.8f, 0.05f, 0.0f, 20.0f);
    binder.Bind("WanderRadius", &wanderRadius, 8.0f, 0.1f, 0.0f, 100.0f);
    binder.Bind("SeparationRadius", &separationRadius, 1.2f, 0.05f, 0.0f, 10.0f);

    binder.Bind("AttackRange", &attackRange, 1.6f, 0.05f, 0.1f, 30.0f);
    binder.Bind("AttackReachMargin", &attackReachMargin, 0.5f, 0.05f, 0.0f, 30.0f);
    binder.Bind("AttackDamage", &attackDamage, 10, 1.0f, 0, 1000);
    binder.Bind("AttackHitTime", &attackHitTime, 0.45f, 0.01f, 0.0f, 1.0f);
    binder.Bind("AttackHalfAngle", &attackHalfAngle, 60.0f, 1.0f, 0.0f, 180.0f);
    binder.Bind("AttackTrackEndTime", &attackTrackEndTime, 0.3f, 0.01f, 0.0f, 1.0f);

    binder.Bind("EmergeAnimSpeed", &emergeAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("WalkAnimSpeed", &walkAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("AttackAnimSpeed", &attackAnimSpeed, 1.0f, 0.01f, 0.1f, 5.0f);
    binder.Bind("AnimBlendTime", &animBlendTime, 0.25f, 0.01f, 0.0f, 2.0f);

    binder.Bind("DamagedSoundVolume", &damagedSoundVolume, 0.6f, 0.01f, 0.0f, 2.0f);
    binder.Bind("SparkSoundVolume", &sparkSoundVolume, 0.5f, 0.01f, 0.0f, 2.0f);

    binder.Bind("HeadRadius", &headRadius, 1.2f, 0.01f, 0.01f, 20.0f);
    binder.Bind("HeadOffset", &headOffset, { 0.0f, 0.6f, 0.0f });
    binder.Bind("HeadDamageRate", &headDamageRate, 2.0f, 0.1f, 0.0f, 20.0f);
    binder.Bind("FlinchAnimSpeed", &flinchAnimSpeed, 0.5f, 0.01f, 0.1f, 5.0f);
    binder.Bind("FlinchEndTime", &flinchEndTime, 0.6f, 0.01f, 0.05f, 1.0f);
    binder.Bind("HeadshotSoundVolume", &headshotSoundVolume, 0.8f, 0.01f, 0.0f, 2.0f);

    burn.Bind(binder, "Burn", ZombieConfig().burn);
    binder.Bind("BurnSparkInterval", &burnSparkInterval, 0.8f, 0.01f, 0.05f, 10.0f);
    binder.Bind("BurnSparkVolume", &burnSparkVolume, 0.35f, 0.01f, 0.0f, 2.0f);
    binder.Bind("BurnSparkHearingDistance", &burnSparkHearingDistance, 30.0f, 0.5f, 0.0f, 500.0f);

    binder.Bind("EmergeSoundVolume", &emergeSoundVolume, 0.7f, 0.01f, 0.0f, 2.0f);
    binder.Bind("EmergeSoundMaxDistance", &emergeSoundMaxDistance, 60.0f, 0.5f, 1.0f, 500.0f);
    binder.Bind("RoarVolume", &roarVolume, 0.6f, 0.01f, 0.0f, 2.0f);
    binder.Bind("RoarMaxDistance", &roarMaxDistance, 60.0f, 0.5f, 1.0f, 500.0f);
    binder.Bind("RoarMinInterval", &roarMinInterval, 6.0f, 0.1f, 0.5f, 120.0f);
    binder.Bind("RoarMaxInterval", &roarMaxInterval, 14.0f, 0.1f, 0.5f, 120.0f);

    binder.Bind("DeathIgniteRadius", &deathIgniteRadius, 4.0f, 0.1f, 0.0f, 50.0f);
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
        binder.Draw("EmergeBuryMargin", "頭のてっぺんより深く埋める分 (スケール 1 のとき)");
        binder.Draw("EmergeDepth", "頭の骨が見つからないときの埋める深さ");
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
        binder.Draw("AttackRange", "攻撃を始める距離");
        binder.Draw("AttackReachMargin", "当たる瞬間に、始める距離より遠くまで届く分");
        binder.Draw("AttackDamage", "ダメージ");
        binder.Draw("AttackHitTime", "当たる瞬間 (アニメーションの 0 ~ 1)");
        binder.Draw("AttackHalfAngle", "当たる範囲 (正面から左右の角度、度)");
        binder.Draw("AttackTrackEndTime", "向きを合わせ続ける所 (アニメーションの 0 ~ 1)");
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

    if (ImGui::TreeNode("撃たれたときの音"))
    {
        binder.Draw("DamagedSoundVolume", "当たる音の大きさ");
        binder.Draw("SparkSoundVolume", "ショートする音の大きさ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("頭"))
    {
        binder.Draw("HeadRadius", "頭の当たり判定の半径");
        binder.Draw("HeadOffset", "頭の骨からのずれ");
        binder.Draw("HeadDamageRate", "頭に当たったときのダメージ (倍)");
        binder.Draw("FlinchAnimSpeed", "ひるむ速さ (humanHit)。遅いほど長くひるむ");
        binder.Draw("FlinchEndTime", "ひるむアニメーションをどこまで流すか (0 ~ 1)");
        binder.Draw("HeadshotSoundVolume", "頭に当たったときの音 (zombieHeadshot) の大きさ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("燃える"))
    {
        burn.Draw(binder, "Burn");
        binder.Draw("BurnSparkInterval", "回路が焼ける音 (zombieSpark) の間隔 (秒)");
        binder.Draw("BurnSparkVolume", "回路が焼ける音の大きさ");
        binder.Draw("BurnSparkHearingDistance", "回路が焼ける音が聞こえる距離 (m)");
        binder.Draw("DeathIgniteRadius", "倒されて爆発したときに草に火がつく半径 (0 でつかない)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("声"))
    {
        binder.Draw("EmergeSoundVolume", "這い出る音 (zombieEmerge) の大きさ");
        binder.Draw("EmergeSoundMaxDistance", "這い出る音が聞こえる距離 (m)");
        binder.Draw("RoarVolume", "吠える声 (zombieRoar) の大きさ");
        binder.Draw("RoarMaxDistance", "吠える声が聞こえる距離 (m)");
        binder.Draw("RoarMinInterval", "次に吠えるまでの最短 (秒)");
        binder.Draw("RoarMaxInterval", "次に吠えるまでの最長 (秒)");
        ImGui::TreePop();
    }
}
