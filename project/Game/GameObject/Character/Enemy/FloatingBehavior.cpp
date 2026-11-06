#include "pch.h"
#include "FloatingBehavior.h"
#include "Enemy.h"
#include "Player.h"
#include "GameDefine.h"
#include "GameObjectManager.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "MathUtils.h"
#include "Terrain.h"

using namespace FS;

namespace
{
    constexpr float kDegToRad = Math::PI / 180.0f;
    const Vector3 kUp = { 0.0f, 1.0f, 0.0f };
}

FloatingBehavior::FloatingBehavior(const FloatingEnemyConfig& config, const Terrain* terrain)
    : config_(config), terrain_(terrain), random_(std::random_device{}())
{
}

FloatingBehavior::~FloatingBehavior()
{
    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(soundHandleID_);
        soundHandleID_ = -1;
    }
}

void FloatingBehavior::Initialize(Enemy* owner)
{
    engine_ = owner->GetEngine();
    auto binder = owner->GetBinder();

    binder->Bind("BasePosition", &basePosition_, basePosition_);
    binder->Bind("Amplitude", &amplitude_, { 3.0f, 2.0f, 0.0f });
    binder->Bind("Frequency", &frequency_, { 0.5f, 1.0f, 0.0f });
    binder->Bind("Phase", &phase_, { 0.0f, 0.0f, 0.0f });
    binder->Bind("PatrolSpeed", &patrolSpeed_, 30.0f, 0.5f, 0.0f, 200.0f);

    RepairThrusters();

    auto moveFireEmitter = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyMoveFire");
    if (moveFireEmitter)
    {
        moveFireEmitterPtr_ = moveFireEmitter.get();
        moveFireEmitterPtr_->SetTargetToFollow(&owner->GetModel()->GetTransform());
        engine_->GetParticleSystem()->AddEmitter(std::move(moveFireEmitter));
    }

    auto moveSmokeEmitter = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyMoveSmoke");
    if (moveSmokeEmitter)
    {
        moveSmokeEmitterPtr_ = moveSmokeEmitter.get();
        moveSmokeEmitterPtr_->SetTargetToFollow(&owner->GetModel()->GetTransform());
        engine_->GetParticleSystem()->AddEmitter(std::move(moveSmokeEmitter));
    }

    auto explosion = engine_->GetParticleSystem()->CreateEmitter("floatingEnemyExplosion");
    explosionEmitterPtr_ = explosion.get();
    if (explosion)
    {
        engine_->GetParticleSystem()->AddEmitter(std::move(explosion));
    }

    owner->GetTransform().translation_ = basePosition_;
    soundHandleID_ = AudioPlayer::GetInstance().Play3D("floatingEnemy", basePosition_, true, 60, 100);
}

void FloatingBehavior::Update(Enemy* owner)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;
    stateTimer_ += deltaTime;

    if (!player_)
    {
        player_ = owner->GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
        // 見つける前に撃った分は銃声として扱わない
        lastPlayerShotCount_ = player_ ? player_->GetShotCount() : 0;
    }

    switch (state_)
    {
    case State::Patrol: UpdatePatrol(owner, deltaTime); break;
    case State::Chase:  UpdateChase(owner, deltaTime);  break;
    case State::LockOn: UpdateLockOn(owner, deltaTime); break;
    case State::Fire:   UpdateFire(owner, deltaTime);   break;
    case State::Crash:  UpdateCrash(owner, deltaTime);  break;
    }

    // Crash の中で Die() されたときはここで終わる
    if (!owner->IsActive())
    {
        return;
    }

    owner->SyncTransform();
    const Vector3& position = owner->GetTransform().translation_;

    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().SetEmitterPosition(soundHandleID_, position);
    }

    // ライトは機体の向きについてくる。狙うときは機体がプレイヤーを向くので、ライトもプレイヤーに当たる
    const Vector3 spotDirection = owner->GetTransform().rotation_.RotateVector(config_.spotLocalDirection).Normalize();
    engine_->GetLightManager()->SubmitSpotLight(
        position,
        config_.spotColor,
        config_.spotIntensity,
        config_.spotDistance,
        spotDirection,
        std::cos(config_.spotAngleDeg * kDegToRad),
        config_.spotVolumetric
    );

    for (Tracer& tracer : tracers_)
    {
        tracer.remainingTime -= deltaTime;
        engine_->GetRendererManager()->SubmitLine(tracer.start, tracer.end, Math::ColorVectorToUint32({ 1.0f, 0.85f, 0.4f, 1.0f }));
    }
    std::erase_if(tracers_, [](const Tracer& tracer) { return tracer.remainingTime <= 0.0f; });

    if (config_.showThrusters)
    {
        DrawThrusterMarkers(owner);
    }
}

void FloatingBehavior::UpdatePatrol(Enemy* owner, float deltaTime)
{
    const Vector3 waveOffset = {
        std::sin(elapsedTime_ * frequency_.x + phase_.x * kDegToRad) * amplitude_.x,
        std::sin(elapsedTime_ * frequency_.y + phase_.y * kDegToRad) * amplitude_.y,
        std::sin(elapsedTime_ * frequency_.z + phase_.z * kDegToRad) * amplitude_.z
    };

    MoveToward(owner, basePosition_ + waveOffset, patrolSpeed_, deltaTime);
    FaceDirection(owner, velocity_, deltaTime);

    if (CanDetectPlayer(owner->GetTransform().translation_))
    {
        // 今いる方向から回り始める
        const Vector3 fromPlayer = owner->GetTransform().translation_ - player_->GetTransform().translation_;
        orbitAngle_ = std::atan2(fromPlayer.z, fromPlayer.x);
        ChangeState(State::Chase);
    }
}

void FloatingBehavior::UpdateChase(Enemy* owner, float deltaTime)
{
    const Vector3& position = owner->GetTransform().translation_;
    const Vector3 playerCenter = player_->GetColliderCenter();
    const float distance = (playerCenter - position).Length();

    if (distance > config_.loseRange)
    {
        ChangeState(State::Patrol);
        return;
    }

    // ときどき周回の向きを変えて読まれにくくする
    orbitChangeTimer_ -= deltaTime;
    if (orbitChangeTimer_ <= 0.0f)
    {
        if (RandomRange(0.0f, 1.0f) < 0.5f)
        {
            orbitDirection_ = -orbitDirection_;
        }
        orbitChangeTimer_ = RandomRange(1.5f, 4.0f);
    }
    orbitAngle_ += config_.orbitSpeed * orbitDirection_ * deltaTime;

    const Vector3 wobble = {
        std::sin(elapsedTime_ * 1.3f) * config_.wobbleAmount,
        std::sin(elapsedTime_ * 2.1f) * config_.wobbleAmount * 0.5f,
        std::cos(elapsedTime_ * 1.7f) * config_.wobbleAmount
    };
    const Vector3 target = player_->GetTransform().translation_
        + Vector3(std::cos(orbitAngle_) * config_.keepDistance, config_.keepHeight, std::sin(orbitAngle_) * config_.keepDistance)
        + wobble;

    MoveToward(owner, target, config_.moveSpeed, deltaTime);
    FaceDirection(owner, playerCenter - position, deltaTime);

    if (stateTimer_ >= config_.attackInterval && distance <= config_.maxFireDistance)
    {
        ChangeState(State::LockOn);
    }
}

void FloatingBehavior::UpdateLockOn(Enemy* owner, float deltaTime)
{
    // 狙っている間はほとんど止まる
    MoveToward(owner, owner->GetTransform().translation_, config_.moveSpeed, deltaTime);

    const Vector3 muzzle = GetMuzzlePosition(owner);
    const Vector3 toPlayer = player_->GetColliderCenter() - muzzle;
    aimDirection_ = toPlayer.Normalize();
    FaceDirection(owner, aimDirection_, deltaTime);

    const float laserLength = std::min(toPlayer.Length(), config_.maxFireDistance);
    engine_->GetRendererManager()->SubmitLine(muzzle, muzzle + aimDirection_ * laserLength, Math::ColorVectorToUint32({ 1.0f, 0.1f, 0.1f, 1.0f }));

    // 撃ち始めたら狙いは固定する。ロックオンの終わりに横へ動けば避けられる
    if (stateTimer_ >= config_.lockOnTime)
    {
        ChangeState(State::Fire);
    }
}

void FloatingBehavior::UpdateFire(Enemy* owner, float deltaTime)
{
    MoveToward(owner, owner->GetTransform().translation_, config_.moveSpeed, deltaTime);

    if (stateTimer_ < config_.shotInterval)
    {
        return;
    }
    stateTimer_ -= config_.shotInterval;

    FireShot(owner);
    ++shotsFired_;

    if (shotsFired_ >= config_.burstCount)
    {
        ChangeState(State::Chase);
    }
}

void FloatingBehavior::UpdateCrash(Enemy* owner, float deltaTime)
{
    constexpr float kSpinSpeed = 8.0f;

    velocity_.y -= config_.crashGravity * deltaTime;
    WorldTransform& transform = owner->GetTransform();
    transform.translation_ += velocity_ * deltaTime;
    transform.SetRotation(Quaternion::FromAxisAngle(kUp, kSpinSpeed * deltaTime) * transform.rotation_);

    // 地面まで落ちたらそこで爆発する
    float groundHeight = 0.0f;
    const bool hasGround = terrain_ && terrain_->GetHeightAt(transform.translation_.x, transform.translation_.z, groundHeight);
    if (hasGround && transform.translation_.y <= groundHeight)
    {
        transform.translation_.y = groundHeight;
        owner->Die();
        return;
    }

    if (stateTimer_ >= config_.crashTime)
    {
        owner->Die();
    }
}

void FloatingBehavior::ChangeState(State state)
{
    state_ = state;
    stateTimer_ = 0.0f;
    shotsFired_ = 0;
}

bool FloatingBehavior::CanDetectPlayer(const Vector3& position)
{
    if (!player_ || !player_->IsControllable())
    {
        return false;
    }

    const float distance = (player_->GetTransform().translation_ - position).Length();

    const int playerShotCount = player_->GetShotCount();
    const bool heardShot = playerShotCount != lastPlayerShotCount_ && distance <= config_.hearingRange;
    lastPlayerShotCount_ = playerShotCount;

    return distance <= config_.sightRange || heardShot;
}

void FloatingBehavior::MoveToward(Enemy* owner, const Vector3& target, float speed, float deltaTime)
{
    Vector3& position = owner->GetTransform().translation_;
    const Vector3 toTarget = target - position;
    const float distance = toTarget.Length();

    // 近づくほど遅くして、目標の周りで行ったり来たりしないようにする
    Vector3 desiredVelocity = { 0.0f, 0.0f, 0.0f };
    if (distance > 0.001f)
    {
        desiredVelocity = toTarget * (std::min(speed, distance * 2.0f) / distance);
    }

    const float blend = std::min(config_.acceleration * deltaTime, 1.0f);
    velocity_ += (desiredVelocity - velocity_) * blend;
    position += velocity_ * deltaTime;
}

void FloatingBehavior::FaceDirection(Enemy* owner, const Vector3& direction, float deltaTime)
{
    if (direction.LengthSq() < 0.0001f)
    {
        return;
    }

    // 真上や真下を向くと LookRotation が崩れるので水平に寄せる
    Vector3 forward = direction.Normalize();
    if (std::abs(forward.y) > 0.95f)
    {
        forward.y = 0.0f;
        if (forward.LengthSq() < 0.0001f)
        {
            return;
        }
        forward = forward.Normalize();
    }

    // モデルは -Z が正面
    WorldTransform& transform = owner->GetTransform();
    const Quaternion targetRotation = Quaternion::LookRotation(-forward, kUp);
    transform.SetRotation(Quaternion::Slerp(transform.rotation_, targetRotation, std::min(config_.turnSpeed * deltaTime, 1.0f)));
}

void FloatingBehavior::FireShot(Enemy* owner)
{
    const Vector3 muzzle = GetMuzzlePosition(owner);
    const Vector3 spread = { RandomRange(-config_.shotSpread, config_.shotSpread), RandomRange(-config_.shotSpread, config_.shotSpread), RandomRange(-config_.shotSpread, config_.shotSpread) };
    const Vector3 shotDirection = (aimDirection_ + spread).Normalize();

    AudioPlayer::GetInstance().Play3D("gunShot", muzzle, false, 50, 150.0f);

    Vector3 shotEnd = muzzle + shotDirection * config_.maxFireDistance;

    // プレイヤーの当たり判定 (箱) を、縦は高さ、横は円として扱う
    const Vector3 playerCenter = player_->GetColliderCenter();
    const Vector3& playerHalfSize = player_->GetColliderSize();
    const float distanceAlongShot = (playerCenter - muzzle).Dot(shotDirection);

    if (distanceAlongShot > 0.0f && distanceAlongShot <= config_.maxFireDistance)
    {
        const Vector3 closestPoint = muzzle + shotDirection * distanceAlongShot;
        const Vector3 offset = playerCenter - closestPoint;
        const float horizontalOffset = std::sqrt(offset.x * offset.x + offset.z * offset.z);
        const float hitRadius = std::max(playerHalfSize.x, playerHalfSize.z);

        if (horizontalOffset <= hitRadius && std::abs(offset.y) <= playerHalfSize.y)
        {
            shotEnd = closestPoint;
            player_->TakeDamage(config_.damagePerShot);
        }
    }

    constexpr float kTracerTime = 0.05f;
    tracers_.push_back({ muzzle, shotEnd, kTracerTime });
}

int FloatingBehavior::ModifyDamage(Enemy* owner, int damage, const Vector3& hitPoint)
{
    if (state_ == State::Crash)
    {
        return damage;
    }

    // 撃たれたら気づく
    if (state_ == State::Patrol && player_)
    {
        ChangeState(State::Chase);
    }

    // 着弾位置をモデルのローカルに戻して、どのスラスターに当たったかを調べる
    const WorldTransform& transform = owner->GetTransform();
    const Vector3 rotatedHit = transform.rotation_.Inverse().RotateVector(hitPoint - transform.translation_);
    const Vector3 localHit = { rotatedHit.x / transform.scale_.x, rotatedHit.y / transform.scale_.y, rotatedHit.z / transform.scale_.z };

    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        if (thrusterHP_[thrusterIndex] <= 0)
        {
            continue;
        }
        if ((localHit - GetThrusterLocalPosition(thrusterIndex)).Length() > config_.thrusterRadius)
        {
            continue;
        }

        thrusterHP_[thrusterIndex] -= damage;
        if (thrusterHP_[thrusterIndex] <= 0)
        {
            DestroyThruster(owner, thrusterIndex);
        }
        return damage;
    }

    return static_cast<int>(static_cast<float>(damage) * config_.bodyDamageRate);
}

void FloatingBehavior::DestroyThruster(Enemy* owner, int thrusterIndex)
{
    thrusterHP_[thrusterIndex] = 0;
    ++destroyedThrusterCount_;
    AudioPlayer::GetInstance().Play3D("floatingEnemyDamaged", owner->GetTransform().translation_, false, 60, 100.0f);

    if (destroyedThrusterCount_ >= config_.thrustersToCrash)
    {
        StartCrash();
    }
}

bool FloatingBehavior::OnHealthDepleted(Enemy* owner)
{
    StartCrash();
    return true;
}

void FloatingBehavior::StartCrash()
{
    if (state_ == State::Crash)
    {
        return;
    }

    // 落ちながら少し流れる
    velocity_.y = std::max(velocity_.y, 0.0f);
    ChangeState(State::Crash);
}

Vector3 FloatingBehavior::GetMuzzlePosition(Enemy* owner) const
{
    const WorldTransform& transform = owner->GetTransform();
    return transform.translation_ + transform.rotation_.RotateVector(config_.muzzleOffset);
}

Vector3 FloatingBehavior::GetThrusterLocalPosition(int thrusterIndex) const
{
    const float signX = (thrusterIndex & 1) ? -1.0f : 1.0f;
    const float signZ = (thrusterIndex & 2) ? -1.0f : 1.0f;
    return { config_.thrusterOffset.x * signX, config_.thrusterOffset.y, config_.thrusterOffset.z * signZ };
}

float FloatingBehavior::RandomRange(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(random_);
}

void FloatingBehavior::DrawThrusterMarkers(Enemy* owner)
{
    const WorldTransform& transform = owner->GetTransform();
    const float markerSize = config_.thrusterRadius * transform.scale_.x;

    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        const Vector3 local = GetThrusterLocalPosition(thrusterIndex);
        const Vector3 scaled = { local.x * transform.scale_.x, local.y * transform.scale_.y, local.z * transform.scale_.z };
        const Vector3 center = transform.translation_ + transform.rotation_.RotateVector(scaled);

        const Vector4 color = thrusterHP_[thrusterIndex] > 0 ? Vector4(0.2f, 1.0f, 0.3f, 1.0f) : Vector4(0.4f, 0.4f, 0.4f, 1.0f);
        const uint32_t packedColor = Math::ColorVectorToUint32(color);

        auto* renderer = engine_->GetRendererManager();
        renderer->SubmitLine(center - Vector3(markerSize, 0.0f, 0.0f), center + Vector3(markerSize, 0.0f, 0.0f), packedColor);
        renderer->SubmitLine(center - Vector3(0.0f, markerSize, 0.0f), center + Vector3(0.0f, markerSize, 0.0f), packedColor);
        renderer->SubmitLine(center - Vector3(0.0f, 0.0f, markerSize), center + Vector3(0.0f, 0.0f, markerSize), packedColor);
    }
}

void FloatingBehavior::OnDeath(Enemy* owner)
{
    if (!engine_ || !owner) return;

    AudioPlayer::GetInstance().Play("floatingEnemyExplosion", false, 40);
    ReleaseEffects();

    if (explosionEmitterPtr_)
    {
        explosionEmitterPtr_->SetPosition(owner->GetTransform().translation_);
        explosionEmitterPtr_->Play();
    }
}

void FloatingBehavior::OnDespawn(Enemy* owner)
{
    ReleaseEffects();

    if (explosionEmitterPtr_)
    {
        explosionEmitterPtr_->Destroy();
        explosionEmitterPtr_ = nullptr;
    }
}

void FloatingBehavior::ReleaseEffects()
{
    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(soundHandleID_);
        soundHandleID_ = -1;
    }

    if (moveFireEmitterPtr_)
    {
        moveFireEmitterPtr_->Stop();
        moveFireEmitterPtr_->Destroy();
        moveFireEmitterPtr_ = nullptr;
    }

    if (moveSmokeEmitterPtr_)
    {
        moveSmokeEmitterPtr_->Stop();
        moveSmokeEmitterPtr_->Destroy();
        moveSmokeEmitterPtr_ = nullptr;
    }
}

void FloatingBehavior::RepairThrusters()
{
    for (int& hp : thrusterHP_)
    {
        hp = config_.thrusterMaxHP;
    }
    destroyedThrusterCount_ = 0;
}

void FloatingBehavior::DebugGui(Enemy* owner)
{
    auto binder = owner->GetBinder();

    constexpr const char* kStateNames[] = { "巡回", "追跡", "ロックオン", "射撃", "墜落" };
    ImGui::Text("状態: %s", kStateNames[static_cast<int>(state_)]);
    ImGui::Text("スラスター HP: %d / %d / %d / %d", thrusterHP_[0], thrusterHP_[1], thrusterHP_[2], thrusterHP_[3]);
    if (ImGui::Button("スラスターを直す"))
    {
        RepairThrusters();
    }

    ImGui::Text("巡回");
    binder->Draw("BasePosition", "基準座標 (中心)");
    binder->Draw("Amplitude", "移動幅");
    binder->Draw("Frequency", "移動スピード");
    binder->Draw("Phase", "波のズレ");
    binder->Draw("PatrolSpeed", "巡回の最高速度");
}

void FloatingEnemyConfig::Bind(PropertyBinder& binder)
{
    binder.Bind("HP", &body.maxHP, 100, 1.0f, 1, 100000);
    binder.Bind("Scale", &body.scale, { 2.0f, 2.0f, 2.0f });
    binder.Bind("ColliderRadius", &body.colliderRadius, 3.25f);
    binder.Bind("ColliderOffset", &body.colliderOffset, { 0.0f, 2.0f, 0.0f });
    binder.Bind("TurnSpeed", &turnSpeed, 6.0f, 0.1f, 0.0f, 50.0f);

    binder.Bind("SightRange", &sightRange, 60.0f, 1.0f, 0.0f, 500.0f);
    binder.Bind("HearingRange", &hearingRange, 120.0f, 1.0f, 0.0f, 500.0f);
    binder.Bind("LoseRange", &loseRange, 150.0f, 1.0f, 0.0f, 1000.0f);

    binder.Bind("MoveSpeed", &moveSpeed, 18.0f, 0.5f, 0.0f, 200.0f);
    binder.Bind("Acceleration", &acceleration, 3.0f, 0.05f, 0.1f, 50.0f);
    binder.Bind("KeepDistance", &keepDistance, 25.0f, 0.5f, 0.0f, 200.0f);
    binder.Bind("KeepHeight", &keepHeight, 10.0f, 0.5f, -50.0f, 200.0f);
    binder.Bind("OrbitSpeed", &orbitSpeed, 0.6f, 0.01f, 0.0f, 10.0f);
    binder.Bind("WobbleAmount", &wobbleAmount, 2.0f, 0.1f, 0.0f, 50.0f);
    binder.Bind("AttackInterval", &attackInterval, 2.5f, 0.05f, 0.0f, 30.0f);

    binder.Bind("LockOnTime", &lockOnTime, 1.0f, 0.05f, 0.0f, 10.0f);
    binder.Bind("BurstCount", &burstCount, 6, 1.0f, 1, 100);
    binder.Bind("ShotInterval", &shotInterval, 0.1f, 0.01f, 0.01f, 2.0f);
    binder.Bind("DamagePerShot", &damagePerShot, 4, 1.0f, 0, 1000);
    binder.Bind("ShotSpread", &shotSpread, 0.03f, 0.001f, 0.0f, 0.5f);
    binder.Bind("MaxFireDistance", &maxFireDistance, 80.0f, 1.0f, 1.0f, 500.0f);
    binder.Bind("MuzzleOffset", &muzzleOffset, { 0.0f, 2.0f, 0.0f });

    binder.Bind("ThrusterOffset", &thrusterOffset, { 1.0f, 1.0f, 1.0f });
    binder.Bind("ThrusterRadius", &thrusterRadius, 0.8f, 0.01f, 0.0f, 20.0f);
    binder.Bind("ThrusterHP", &thrusterMaxHP, 30, 1.0f, 1, 10000);
    binder.Bind("BodyDamageRate", &bodyDamageRate, 0.3f, 0.01f, 0.0f, 1.0f);
    binder.Bind("ThrustersToCrash", &thrustersToCrash, 2, 1.0f, 1, 4);

    binder.Bind("CrashTime", &crashTime, 5.0f, 0.05f, 0.0f, 10.0f);
    binder.Bind("CrashGravity", &crashGravity, 30.0f, 0.5f, 0.0f, 200.0f);

    binder.BindColor("SpotColor", &spotColor, { 0.076f, 0.216f, 1.0f, 1.0f });
    binder.Bind("SpotIntensity", &spotIntensity, 30.85f);
    binder.Bind("SpotDistance", &spotDistance, 28.24f);
    binder.Bind("SpotAngle", &spotAngleDeg, 28.13f);
    binder.Bind("SpotVolumetric", &spotVolumetric, 200.0f);
    binder.Bind("SpotLocalDirection", &spotLocalDirection, { 0.0f, -0.3f, -1.0f });
}

void FloatingEnemyConfig::Draw(PropertyBinder& binder)
{
    if (ImGui::TreeNode("本体"))
    {
        binder.Draw("HP", "HP");
        binder.Draw("Scale", "スケール");
        binder.Draw("ColliderRadius", "当たり判定の半径");
        binder.Draw("ColliderOffset", "当たり判定の位置");
        binder.Draw("TurnSpeed", "向きを変える速さ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("発見"))
    {
        binder.Draw("SightRange", "気づく距離");
        binder.Draw("HearingRange", "銃声に気づく距離");
        binder.Draw("LoseRange", "見失う距離");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("追跡"))
    {
        binder.Draw("MoveSpeed", "最高速度");
        binder.Draw("Acceleration", "加速 (大きいほど機敏)");
        binder.Draw("KeepDistance", "プレイヤーとの水平距離");
        binder.Draw("KeepHeight", "プレイヤーより上の高さ");
        binder.Draw("OrbitSpeed", "周回の速さ (ラジアン/秒)");
        binder.Draw("WobbleAmount", "ふらつき");
        binder.Draw("AttackInterval", "攻撃の間隔 (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("攻撃"))
    {
        binder.Draw("LockOnTime", "ロックオン時間 (秒)");
        binder.Draw("BurstCount", "連射数");
        binder.Draw("ShotInterval", "連射の間隔 (秒)");
        binder.Draw("DamagePerShot", "1 発のダメージ");
        binder.Draw("ShotSpread", "ばらつき (ラジアン)");
        binder.Draw("MaxFireDistance", "射程");
        binder.Draw("MuzzleOffset", "銃口の位置");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("弱点・墜落"))
    {
        ImGui::Checkbox("スラスターの位置を表示", &showThrusters);
        binder.Draw("ThrusterOffset", "スラスターの位置 (1 基目)");
        binder.Draw("ThrusterRadius", "スラスターの半径");
        binder.Draw("ThrusterHP", "スラスター 1 基の HP");
        binder.Draw("BodyDamageRate", "本体へのダメージ倍率");
        binder.Draw("ThrustersToCrash", "墜落するまでに壊す数");
        binder.Draw("CrashTime", "地面に届かないときに爆発するまで (秒)");
        binder.Draw("CrashGravity", "墜落の重力");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("スポットライト"))
    {
        binder.Draw("SpotColor", "色");
        binder.Draw("SpotIntensity", "ライト輝度");
        binder.Draw("SpotDistance", "届く距離");
        binder.Draw("SpotAngle", "照射角");
        binder.Draw("SpotVolumetric", "ボリュームフォグ輝度");
        binder.Draw("SpotLocalDirection", "照射方向 (機体から見て。-Z が正面)");
        ImGui::TreePop();
    }
}
