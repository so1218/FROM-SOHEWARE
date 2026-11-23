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
#include "CollisionConfig.h"
#include "RendererManager.h"
#include "TreeField.h"

using namespace FS;

FloatingBehavior::FloatingBehavior(const FloatingEnemyConfig& config, const Terrain* terrain, const TreeField* treeField, const Model* thrusterSourceModel,
    const Vector3& spawnPosition, const Vector3& firstWanderTarget)
    : config_(config), thrusterSourceModel_(thrusterSourceModel), terrain_(terrain), treeField_(treeField), random_(std::random_device{}()),
    spawnPosition_(spawnPosition), wanderTarget_(firstWanderTarget), hasWanderTarget_(true)
{
}

FloatingBehavior::~FloatingBehavior()
{
    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(soundHandleID_);
        soundHandleID_ = -1;
    }

    // 吸い込まれている途中の粒が、消えた銃口の位置を読まないようにする
    if (engine_)
    {
        engine_->GetParticleSystem()->ReleaseAttractionTarget(&chargeTarget_);
    }
}

void FloatingBehavior::Initialize(Enemy* owner)
{
    engine_ = owner->GetEngine();
    // スラスターは本体とは別の箱で、弾のレイで当たったかを調べる
    for (Collider*& collider : thrusterColliders_)
    {
        collider = owner->AddBoxCollider(config_.thrusterColliderSize);
        collider->SetCollisionLayer(CollisionLayer::Enemy);
        collider->SetRaycastOnly(true);
    }

    // マテリアルは全機で共有する
    for (std::unique_ptr<Model>& model : thrusterModels_)
    {
        model = std::make_unique<Model>(engine_, "enemyThruster");
        model->ShareMaterialsFrom(thrusterSourceModel_);
    }
    RepairThrusters();

    ParticleSystem* particleSystem = engine_->GetParticleSystem();
    moveFireEmitterPtr_ = particleSystem->SpawnEmitter("floatingEnemyMoveFire");
    moveFireEmitterPtr_->SetTargetToFollow(&owner->GetModel()->GetTransform());
    explosionEmitterPtr_ = particleSystem->SpawnEmitter("floatingEnemyExplosion");
    deathSparkEmitterPtr_ = particleSystem->SpawnEmitter("enemyDeathSpark");
    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        thrusterFireEmitterPtrs_[thrusterIndex] = particleSystem->SpawnEmitter("floatingThrusterFire");
        if (thrusterFireEmitterPtrs_[thrusterIndex])
        {
            thrusterFireEmitterPtrs_[thrusterIndex]->SetTargetToFollow(&thrusterModels_[thrusterIndex]->GetTransform());
        }
    }
    tracerEmitterPtr_ = particleSystem->SpawnEmitter("floatingTracer");
    chargeEmitterPtr_ = particleSystem->SpawnEmitter("floatingCharge");
    if (chargeEmitterPtr_)
    {
        chargeEmitterPtr_->SetAttractionTarget(&chargeTarget_);
    }

    // 最初の目的地の方を向いて出てくる
    WorldTransform& transform = owner->GetTransform();
    transform.translation_ = spawnPosition_;
    const Vector3 toTarget = { wanderTarget_.x - spawnPosition_.x, 0.0f, wanderTarget_.z - spawnPosition_.z };
    if (toTarget.LengthSq() > 0.0001f)
    {
        transform.SetRotation(Quaternion::LookRotation(-toTarget.Normalize(), { 0.0f, 1.0f, 0.0f }));
    }
    owner->SyncTransform();
    UpdateThrusters(owner);

    currentSpotColor_ = config_.spotColor;
    currentSpotAngleDeg_ = config_.spotAngleDeg;

    soundHandleID_ = AudioPlayer::GetInstance().Play3D("floatingEnemy", spawnPosition_, { .volume = 0.6f, .maxDistance = 100.0f, .isLoop = true });
}

void FloatingBehavior::Update(Enemy* owner)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;
    stateTimer_ += deltaTime;
    propellerAngle_ = std::fmod(propellerAngle_ + config_.propellerSpeed * deltaTime, Math::PI * 2.0f);

    if (!player_)
    {
        player_ = owner->GetManager()->FindObjectWithTag<Player>(ObjectTag::Player);
        // 見つける前に撃った分は銃声として扱わない
        lastPlayerShotCount_ = player_ ? player_->GetShotCount() : 0;
    }

    // 敵に無視される設定にしたら、追いかけるのをやめる
    if (player_ && player_->IsIgnoredByEnemies() && (state_ == State::Chase || state_ == State::LockOn || state_ == State::Fire))
    {
        ChangeState(State::Patrol);
    }

    // 狙っている間や撃っている間も数え、隠れ続ければ見失う
    if (player_ && (state_ == State::Chase || state_ == State::LockOn || state_ == State::Fire))
    {
        const float distance = (player_->GetColliderCenter() - owner->GetTransform().translation_).Length();
        hiddenTimer_ = IsPlayerHiddenFrom(distance) ? hiddenTimer_ + deltaTime : 0.0f;
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
    UpdateThrusters(owner);
    const Vector3& position = owner->GetTransform().translation_;

    if (soundHandleID_ >= 0)
    {
        AudioPlayer::GetInstance().SetEmitterPosition(soundHandleID_, position);
    }

    // ライトの色で状態を伝える。巡回は普段の色、見つけたら警戒の色、狙っている間は絞って狙いの先へ当てる
    const bool isAiming = state_ == State::LockOn || state_ == State::Fire;
    Vector4 targetSpotColor = config_.spotColor;
    if (isAiming)
    {
        targetSpotColor = config_.spotLockColor;
    }
    else if (state_ == State::Chase)
    {
        targetSpotColor = config_.spotAlertColor;
    }
    const float targetSpotAngleDeg = isAiming ? config_.spotLockAngleDeg : config_.spotAngleDeg;
    const float spotBlend = 1.0f - std::exp(-config_.spotChangeSpeed * deltaTime);
    currentSpotColor_ = currentSpotColor_ + (targetSpotColor - currentSpotColor_) * spotBlend;
    currentSpotAngleDeg_ += (targetSpotAngleDeg - currentSpotAngleDeg_) * spotBlend;

    // 普段は機体の向きについてくる。狙っている間は狙いの向きへ当てる
    const Vector3 spotDirection = isAiming
        ? aimDirection_
        : owner->GetTransform().rotation_.RotateVector(config_.spotLocalDirection).Normalize();
    engine_->GetLightManager()->SubmitSpotLight(
        position,
        currentSpotColor_,
        config_.spotIntensity,
        config_.spotDistance,
        spotDirection,
        std::cos(currentSpotAngleDeg_ * Math::PI / 180.0f),
        config_.spotVolumetric
    );

    // 銃口の位置を合わせるときの印
    if (config_.showMuzzleMarker)
    {
        // 3 本の軸の交わる所が銃口
        constexpr float kMarkerSize = 0.5f;
        const Vector3 muzzle = GetMuzzlePosition(owner);
        const uint32_t markerColor = Math::ColorVectorToUint32({ 0.2f, 1.0f, 0.2f, 1.0f });
        RendererManager* rendererManager = engine_->GetRendererManager();
        for (const Vector3& axis : { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) })
        {
            rendererManager->SubmitLine(muzzle - axis * kMarkerSize, muzzle + axis * kMarkerSize, markerColor);
        }
    }
}

void FloatingBehavior::UpdatePatrol(Enemy* owner, float deltaTime)
{
    // 目的地に着いたとみなす水平距離
    constexpr float kArriveDistance = 4.0f;

    const Vector3& position = owner->GetTransform().translation_;
    if (!hasWanderTarget_)
    {
        ChooseWanderTarget(position);
    }

    // 地形の起伏に沿って、今いる所の地面から一定の高さを飛ぶ。上空から入ってきたときは、ここへ降りてくる
    const float cruiseHeight = GetGroundHeight(position.x, position.z) + config_.wanderHeight;

    if (wanderPauseTimer_ > 0.0f)
    {
        // 止まって、ライトで周りを照らしながらゆっくり見回す
        MoveToward(owner, { position.x, cruiseHeight, position.z }, config_.wanderSpeed, deltaTime);
        scanAngle_ += scanDirection_ * config_.scanTurnSpeed * deltaTime;
        FaceDirection(owner, { std::sin(scanAngle_), 0.0f, std::cos(scanAngle_) }, deltaTime);

        wanderPauseTimer_ -= deltaTime;
        if (wanderPauseTimer_ <= 0.0f)
        {
            ChooseWanderTarget(position);
        }
    }
    else
    {
        MoveToward(owner, { wanderTarget_.x, cruiseHeight, wanderTarget_.z }, config_.wanderSpeed, deltaTime);
        FaceDirection(owner, { velocity_.x, 0.0f, velocity_.z }, deltaTime);

        const float offsetX = wanderTarget_.x - position.x;
        const float offsetZ = wanderTarget_.z - position.z;
        if (offsetX * offsetX + offsetZ * offsetZ < kArriveDistance * kArriveDistance)
        {
            wanderPauseTimer_ = RandomRange(config_.wanderPauseMinTime, std::max(config_.wanderPauseMinTime, config_.wanderPauseMaxTime));
            const Vector3 forward = owner->GetTransform().rotation_.RotateVector({ 0.0f, 0.0f, -1.0f });
            scanAngle_ = std::atan2(forward.x, forward.z);
            scanDirection_ = RandomRange(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;
        }
    }

    if (CanDetectPlayer(position))
    {
        StartChase(owner);
    }
}

void FloatingBehavior::ChooseWanderTarget(const Vector3& position)
{
    // フィールドの端からこれより内側を選ぶ
    constexpr float kFieldEdgeMargin = 30.0f;
    constexpr int kAttemptCount = 8;

    hasWanderTarget_ = true;
    wanderPauseTimer_ = 0.0f;

    // プレイヤーから離れすぎたら、プレイヤーのいる辺りを回るように戻す
    Vector3 origin = position;
    if (player_)
    {
        const Vector3& playerPosition = player_->GetTransform().translation_;
        const float offsetX = playerPosition.x - position.x;
        const float offsetZ = playerPosition.z - position.z;
        if (offsetX * offsetX + offsetZ * offsetZ > config_.wanderLeashDistance * config_.wanderLeashDistance)
        {
            origin = playerPosition;
        }
    }

    if (!terrain_)
    {
        wanderTarget_ = origin;
        return;
    }

    const Vector2 center = terrain_->GetCenterXZ();
    const Vector2 halfSize = terrain_->GetSizeXZ() * 0.5f - Vector2(kFieldEdgeMargin, kFieldEdgeMargin);
    const float minStep = std::min(config_.wanderMinStep, config_.wanderMaxStep);
    const float maxStep = std::max(config_.wanderMinStep, config_.wanderMaxStep);
    for (int attempt = 0; attempt < kAttemptCount; ++attempt)
    {
        const float angle = RandomRange(0.0f, Math::PI * 2.0f);
        const float step = RandomRange(minStep, maxStep);
        const Vector3 candidate = { origin.x + std::cos(angle) * step, 0.0f, origin.z + std::sin(angle) * step };
        if (std::abs(candidate.x - center.x) <= halfSize.x && std::abs(candidate.z - center.y) <= halfSize.y)
        {
            wanderTarget_ = candidate;
            return;
        }
    }

    // 端に追い詰められて見つからなければ、フィールドの中央へ向かう
    wanderTarget_ = { center.x, 0.0f, center.y };
}

float FloatingBehavior::GetGroundHeight(float x, float z) const
{
    float groundHeight = 0.0f;
    if (terrain_ && terrain_->GetHeightAt(x, z, groundHeight))
    {
        return groundHeight;
    }
    return 0.0f;
}

void FloatingBehavior::UpdateChase(Enemy* owner, float deltaTime)
{
    const Vector3& position = owner->GetTransform().translation_;
    const Vector3 playerCenter = player_->GetColliderCenter();
    const float distance = (playerCenter - position).Length();

    if (distance > config_.loseRange || hiddenTimer_ >= config_.hiddenLoseTime)
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

    // 見えていない相手は狙わない
    if (stateTimer_ >= config_.attackInterval && distance <= config_.maxFireDistance && !IsPlayerHiddenFrom(distance))
    {
        StartLockOn(owner);
    }
}

void FloatingBehavior::UpdateLockOn(Enemy* owner, float deltaTime)
{
    // 狙っている間はほとんど止まる
    MoveToward(owner, owner->GetTransform().translation_, config_.moveSpeed, deltaTime);

    // 追いかける間 → 狙いを止める間 → 撃つ、の順に進む
    const float lockOnTime = std::max(config_.lockOnTime, 0.01f);
    const float trackEndTime = std::max(lockOnTime - config_.lockFreezeTime, 0.0f);
    const bool isLocked = stateTimer_ >= trackEndTime;
    const Vector3 muzzle = GetMuzzlePosition(owner);

    // 撃つ直前ほど激しく吸い込み、それ自体を「もうすぐ撃つ」合図にする
    chargeTarget_.translation_ = muzzle;
    if (chargeEmitterPtr_)
    {
        const float chargeRatio = std::min(stateTimer_ / lockOnTime, 1.0f);
        chargeEmitterPtr_->SetPosition(muzzle);
        chargeEmitterPtr_->SetEmissionRateMultiplier(config_.chargeRateStart + (config_.chargeRateEnd - config_.chargeRateStart) * chargeRatio);
    }

    // 少し遅れて追いかけるので、横へ動き続ければ狙いをずらせる
    if (!isLocked)
    {
        const Vector3 toPlayer = player_->GetColliderCenter() - muzzle;
        if (toPlayer.LengthSq() > 0.0001f)
        {
            const float blend = 1.0f - std::exp(-config_.aimTrackSharpness * deltaTime);
            aimDirection_ = (aimDirection_ + (toPlayer.Normalize() - aimDirection_) * blend).Normalize();
        }
    }
    FaceDirection(owner, aimDirection_, deltaTime);

    if (stateTimer_ >= lockOnTime)
    {
        ChangeState(State::Fire);
    }
}

void FloatingBehavior::StartLockOn(Enemy* owner)
{
    // 今向いている先から追いかけ始める
    const Vector3 muzzle = GetMuzzlePosition(owner);
    const Vector3 toPlayer = player_->GetColliderCenter() - muzzle;
    const Vector3 forward = owner->GetTransform().rotation_.RotateVector({ 0.0f, 0.0f, -1.0f });
    aimDirection_ = (forward.LengthSq() > 0.0001f) ? forward.Normalize() : toPlayer.Normalize();

    PlayIfLoaded3D("floatingLockOn", muzzle, config_.lockOnSoundVolume);
    ChangeState(State::LockOn);

    chargeTarget_.translation_ = muzzle;
    if (chargeEmitterPtr_)
    {
        chargeEmitterPtr_->SetPosition(muzzle);
        chargeEmitterPtr_->SetEmissionRateMultiplier(config_.chargeRateStart);
        chargeEmitterPtr_->Play();
    }
}

bool FloatingBehavior::FindGroundHit(const Vector3& start, const Vector3& direction, float maxDistance, Vector3& outPoint) const
{
    if (!terrain_)
    {
        return false;
    }

    // 1m ずつ進めて地面の下に入った所を探し、前の点との間を割合で詰める
    constexpr float kStep = 1.0f;
    float previousDistance = 0.0f;
    float previousHeightAbove = start.y - GetGroundHeight(start.x, start.z);
    for (float distance = kStep; distance <= maxDistance; distance += kStep)
    {
        const Vector3 point = start + direction * distance;
        const float heightAbove = point.y - GetGroundHeight(point.x, point.z);
        if (heightAbove <= 0.0f)
        {
            const float ratio = previousHeightAbove / std::max(previousHeightAbove - heightAbove, 0.0001f);
            outPoint = start + direction * (previousDistance + (distance - previousDistance) * ratio);
            return true;
        }
        previousDistance = distance;
        previousHeightAbove = heightAbove;
    }
    return false;
}

void FloatingBehavior::PlayIfLoaded3D(const std::string& name, const Vector3& position, float volume) const
{
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    if (audioPlayer.HasSound(name))
    {
        audioPlayer.Play3D(name, position, { .volume = volume, .maxDistance = 150.0f });
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
    transform.SetRotation(Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, kSpinSpeed * deltaTime) * transform.rotation_);

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

    // ためるのは狙っている間だけ。撃ち始めたら、残りの粒は銃口へ吸い込まれて消える
    if (state != State::LockOn && chargeEmitterPtr_)
    {
        chargeEmitterPtr_->Stop();
    }

    // 見失ったら、その場所から徘徊し直す
    if (state == State::Patrol)
    {
        hasWanderTarget_ = false;
        wanderPauseTimer_ = 0.0f;
    }
}

void FloatingBehavior::StartChase(Enemy* owner)
{
    // 今いる方向から回り始める
    const Vector3 fromPlayer = owner->GetTransform().translation_ - player_->GetTransform().translation_;
    orbitAngle_ = std::atan2(fromPlayer.z, fromPlayer.x);
    hiddenTimer_ = 0.0f;
    ChangeState(State::Chase);

    AudioPlayer::GetInstance().Play("floatingScream", config_.screamVolume);
}

bool FloatingBehavior::CanDetectPlayer(const Vector3& position)
{
    if (!player_ || !player_->CanBeNoticed())
    {
        return false;
    }

    const float distance = (player_->GetTransform().translation_ - position).Length();

    const int playerShotCount = player_->GetShotCount();
    const bool heardShot = playerShotCount != lastPlayerShotCount_ && distance <= config_.hearingRange;
    lastPlayerShotCount_ = playerShotCount;

    // 隠れていると、近くまで来ないと気づかない
    const float sightRange = player_->IsHidden() ? std::min(config_.hiddenSightRange, config_.sightRange) : config_.sightRange;
    return distance <= sightRange || heardShot;
}

bool FloatingBehavior::IsPlayerHiddenFrom(float distance) const
{
    return player_ && player_->IsHidden() && distance > config_.hiddenSightRange;
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
    const Quaternion targetRotation = Quaternion::LookRotation(-forward, { 0.0f, 1.0f, 0.0f });
    transform.SetRotation(Quaternion::Slerp(transform.rotation_, targetRotation, std::min(config_.turnSpeed * deltaTime, 1.0f)));
}

void FloatingBehavior::FireShot(Enemy* owner)
{
    const Vector3 muzzle = GetMuzzlePosition(owner);
    const Vector3 spread = { RandomRange(-config_.shotSpread, config_.shotSpread), RandomRange(-config_.shotSpread, config_.shotSpread), RandomRange(-config_.shotSpread, config_.shotSpread) };
    const Vector3 shotDirection = (aimDirection_ + spread).Normalize();

    // プレイヤーの銃と聞き分けられるよう、敵専用の銃声にする
    AudioPlayer::GetInstance().Play3D("floatingEnemyShot", muzzle, { .volume = config_.shotSoundVolume, .maxDistance = 150.0f });

    // 木に当たれば弾はそこで止まり、奥のプレイヤーには当たらない
    float shotDistance = config_.maxFireDistance;
    TreeRaycastHit treeHit;
    if (treeField_ && treeField_->Raycast(muzzle, shotDirection, config_.maxFireDistance, &treeHit))
    {
        shotDistance = treeHit.distance;
    }

    Vector3 shotEnd = muzzle + shotDirection * shotDistance;
    bool hasHitPlayer = false;

    // プレイヤーの当たり判定 (箱) を、縦は高さ、横は円として扱う
    const Vector3 playerCenter = player_->GetColliderCenter();
    const Vector3 playerHalfSize = player_->GetColliderHalfSize();
    const float distanceAlongShot = (playerCenter - muzzle).Dot(shotDirection);

    if (distanceAlongShot > 0.0f && distanceAlongShot <= shotDistance)
    {
        const Vector3 closestPoint = muzzle + shotDirection * distanceAlongShot;
        const Vector3 offset = playerCenter - closestPoint;
        const float horizontalOffset = std::sqrt(offset.x * offset.x + offset.z * offset.z);
        const float hitRadius = std::max(playerHalfSize.x, playerHalfSize.z);

        if (horizontalOffset <= hitRadius && std::abs(offset.y) <= playerHalfSize.y)
        {
            shotEnd = closestPoint;
            hasHitPlayer = true;
            player_->TakeDamage(config_.damagePerShot);
        }
    }

    // 外れた弾は地面で止める
    Vector3 groundHit;
    if (!hasHitPlayer && FindGroundHit(muzzle, shotDirection, shotDistance, groundHit))
    {
        shotEnd = groundHit;
    }

    // 光る弾の筋。弾が止まる所で消えるよう寿命を決める
    const Quaternion shotRotation = Quaternion::LookRotation(shotDirection, { 0.0f, 1.0f, 0.0f });
    if (tracerEmitterPtr_)
    {
        const float tracerDistance = (shotEnd - muzzle).Length();
        const float tracerSpeed = engine_->GetParticleSystem()->GetConfig("floatingTracer").velocity.speed;
        if (tracerSpeed > 0.0f)
        {
            tracerEmitterPtr_->lifetime_ = tracerDistance / tracerSpeed;
        }
        tracerEmitterPtr_->SetPosition(muzzle);
        tracerEmitterPtr_->SetRotation(shotRotation);
        tracerEmitterPtr_->Play();
    }
}

int FloatingBehavior::ModifyDamage(Enemy* owner, int damage, const Collider* hitCollider)
{
    if (state_ == State::Crash)
    {
        return damage;
    }

    // 撃たれたら気づく
    if (state_ == State::Patrol && player_ && player_->CanBeNoticed())
    {
        StartChase(owner);
    }

    // 壊れたスラスターのコライダーは無効にしてあるので、当たるのは残っているものだけ
    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        if (thrusterColliders_[thrusterIndex] != hitCollider)
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
    thrusterColliders_[thrusterIndex]->SetEnable(false);   // 同じフレームの次の弾が当たらないよう、すぐ外す
    ++destroyedThrusterCount_;

    if (thrusterFireEmitterPtrs_[thrusterIndex])
    {
        thrusterFireEmitterPtrs_[thrusterIndex]->Play();
    }
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    const Vector3& thrusterPosition = thrusterModels_[thrusterIndex]->GetTransform().translation_;
    audioPlayer.Play3D("floatingEnemyDamaged", owner->GetTransform().translation_, { .volume = 0.6f, .maxDistance = 100.0f });
    audioPlayer.Play3D("floatingEnemyExplosion", thrusterPosition, { .volume = config_.thrusterExplosionVolume, .maxDistance = 150.0f });

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
    // スラスターと同じく、機体の大きさを変えても銃口がずれないようにスケールを掛ける
    return transform.translation_ + transform.rotation_.RotateVector(Vector3::Scale(config_.muzzleOffset, transform.scale_));
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

// 調整中に ImGui で位置や大きさを変えても、すぐ当たり判定と見た目に反映されるよう毎フレーム設定する
void FloatingBehavior::UpdateThrusters(Enemy* owner)
{
    const WorldTransform& body = owner->GetTransform();

    // コライダーは回転だけ反映するので、機体のスケールはここで掛けておく
    const Vector3 colliderSize = Vector3::Scale(config_.thrusterColliderSize, body.scale_);
    const Vector3 modelScale = Vector3::Scale(config_.thrusterScale, body.scale_);

    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        const Vector3 offset = Vector3::Scale(GetThrusterLocalPosition(thrusterIndex), body.scale_);
        const bool isAlive = thrusterHP_[thrusterIndex] > 0;

        Collider* collider = thrusterColliders_[thrusterIndex];
        collider->SetCenterOffset(offset);
        collider->SetSize(colliderSize);
        collider->SetEnable(isAlive);
        collider->SetColor(isAlive ? Vector4(0.2f, 1.0f, 0.3f, 1.0f) : Vector4(0.4f, 0.4f, 0.4f, 1.0f));

        // 対角どうしを同じ向きに回し、隣とは逆に回す
        const bool isClockwise = ((thrusterIndex & 1) != 0) != ((thrusterIndex & 2) != 0);
        const float spinAngle = isClockwise ? -propellerAngle_ : propellerAngle_;

        // 機体の上向きの軸で回す
        WorldTransform& transform = thrusterModels_[thrusterIndex]->GetTransform();
        transform.translation_ = body.translation_ + body.rotation_.RotateVector(offset);
        transform.SetRotation(body.rotation_ * Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, spinAngle));
        transform.scale_ = modelScale;
    }
}

void FloatingBehavior::Draw(Enemy* owner)
{
    // 壊れたスラスターは描かない
    for (int thrusterIndex = 0; thrusterIndex < kThrusterCount; ++thrusterIndex)
    {
        if (thrusterHP_[thrusterIndex] > 0)
        {
            thrusterModels_[thrusterIndex]->Draw();
        }
    }
}

void FloatingBehavior::OnDeath(Enemy* owner)
{
    AudioPlayer::GetInstance().Play("floatingEnemyExplosion", 0.4f);
    ReleaseEffects();

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

    // 地面の近くで爆発したら草に火がつく
    const Vector3& position = owner->GetTransform().translation_;
    float groundHeight = 0.0f;
    if (config_.deathIgniteRadius > 0.0f && terrain_ && terrain_->GetHeightAt(position.x, position.z, groundHeight) &&
        position.y - groundHeight <= config_.deathIgniteGroundReach)
    {
        engine_->GetRendererManager()->SubmitGrassIgnition({ position.x, groundHeight, position.z }, config_.deathIgniteRadius);
    }
}

void FloatingBehavior::OnTakeDamage(Enemy* owner, int damage, const Vector3& hitPoint, const Vector3& hitNormal)
{
    AudioPlayer::GetInstance().Play("floatingEnemyDamaged", 0.5f);
}

void FloatingBehavior::OnDespawn(Enemy* owner)
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

    // 撃った演出は飛んでいる途中の粒が消えるまで残したいので、倒れたときではなく片付けるときに消す
    for (ParticleEmitter** emitter : { &tracerEmitterPtr_, &chargeEmitterPtr_ })
    {
        if (*emitter)
        {
            (*emitter)->Destroy();
            *emitter = nullptr;
        }
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

    // 倒れたら火も消す
    for (ParticleEmitter*& emitter : thrusterFireEmitterPtrs_)
    {
        if (emitter)
        {
            emitter->Destroy();
            emitter = nullptr;
        }
    }
}

void FloatingBehavior::RepairThrusters()
{
    for (int& hp : thrusterHP_)
    {
        hp = config_.thrusterMaxHP;
    }
    destroyedThrusterCount_ = 0;

    for (ParticleEmitter* emitter : thrusterFireEmitterPtrs_)
    {
        if (emitter)
        {
            emitter->Stop();
        }
    }
}

void FloatingBehavior::DebugGui(Enemy* owner)
{
    constexpr const char* kStateNames[] = { "巡回", "追跡", "ロックオン", "射撃", "墜落" };
    ImGui::Text("状態: %s", kStateNames[static_cast<int>(state_)]);
    ImGui::Text("スラスター HP: %d / %d / %d / %d", thrusterHP_[0], thrusterHP_[1], thrusterHP_[2], thrusterHP_[3]);
    ImGui::Text("プレイヤーが見えていない時間: %.1f 秒", hiddenTimer_);
    if (ImGui::Button("スラスターを直す"))
    {
        RepairThrusters();
    }

    ImGui::Text("目的地: (%.0f, %.0f)%s", wanderTarget_.x, wanderTarget_.z, wanderPauseTimer_ > 0.0f ? " で見回し中" : "");
}

void FloatingEnemyConfig::Bind(PropertyBinder& binder)
{
    binder.Bind("HP", &body.maxHP, 100, 1.0f, 1, 100000);
    binder.Bind("Scale", &body.scale, { 2.0f, 2.0f, 2.0f });
    binder.Bind("ColliderSize", &body.colliderSize, { 6.5f, 6.5f, 6.5f });
    binder.Bind("ColliderOffset", &body.colliderOffset, { 0.0f, 2.0f, 0.0f });
    binder.Bind("TurnSpeed", &turnSpeed, 6.0f, 0.1f, 0.0f, 50.0f);

    binder.Bind("IsSpawnEnabled", &isSpawnEnabled, true);
    binder.Bind("MaxAliveCount", &maxAliveCount, 3, 1.0f, 0, 10);
    binder.Bind("SpawnInterval", &spawnInterval, 40.0f, 0.5f, 0.0f, 600.0f);
    binder.Bind("SpawnMinDistance", &spawnMinDistance, 100.0f, 1.0f, 0.0f, 1000.0f);
    binder.Bind("SpawnMaxDistance", &spawnMaxDistance, 160.0f, 1.0f, 0.0f, 1000.0f);
    binder.Bind("SpawnHeight", &spawnHeight, 60.0f, 1.0f, 0.0f, 500.0f);
    binder.Bind("InitialMinDistance", &initialMinDistance, 80.0f, 1.0f, 0.0f, 1000.0f);

    binder.Bind("WanderSpeed", &wanderSpeed, 10.0f, 0.1f, 0.0f, 200.0f);
    binder.Bind("WanderHeight", &wanderHeight, 15.0f, 0.5f, 0.0f, 200.0f);
    binder.Bind("WanderMinStep", &wanderMinStep, 30.0f, 1.0f, 1.0f, 1000.0f);
    binder.Bind("WanderMaxStep", &wanderMaxStep, 80.0f, 1.0f, 1.0f, 1000.0f);
    binder.Bind("WanderLeashDistance", &wanderLeashDistance, 180.0f, 1.0f, 10.0f, 2000.0f);
    binder.Bind("WanderPauseMinTime", &wanderPauseMinTime, 2.0f, 0.1f, 0.0f, 60.0f);
    binder.Bind("WanderPauseMaxTime", &wanderPauseMaxTime, 5.0f, 0.1f, 0.0f, 60.0f);
    binder.Bind("ScanTurnSpeed", &scanTurnSpeed, 0.5f, 0.01f, 0.0f, 10.0f);

    binder.Bind("SightRange", &sightRange, 60.0f, 1.0f, 0.0f, 500.0f);
    binder.Bind("HearingRange", &hearingRange, 120.0f, 1.0f, 0.0f, 500.0f);
    binder.Bind("LoseRange", &loseRange, 150.0f, 1.0f, 0.0f, 1000.0f);
    binder.Bind("HiddenSightRange", &hiddenSightRange, 10.0f, 0.5f, 0.0f, 500.0f);
    binder.Bind("HiddenLoseTime", &hiddenLoseTime, 4.0f, 0.1f, 0.0f, 60.0f);
    binder.Bind("ScreamVolume", &screamVolume, 0.6f, 0.01f, 0.0f, 2.0f);

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

    binder.Bind("AimTrackSharpness", &aimTrackSharpness, 6.0f, 0.1f, 0.1f, 100.0f);
    binder.Bind("LockFreezeTime", &lockFreezeTime, 0.3f, 0.01f, 0.0f, 5.0f);

    binder.Bind("ChargeRateStart", &chargeRateStart, 0.3f, 0.01f, 0.0f, 20.0f);
    binder.Bind("ChargeRateEnd", &chargeRateEnd, 2.0f, 0.01f, 0.0f, 20.0f);
    binder.Bind("ShowMuzzleMarker", &showMuzzleMarker, false);

    binder.Bind("ShotSoundVolume", &shotSoundVolume, 0.5f, 0.01f, 0.0f, 2.0f);
    binder.Bind("LockOnSoundVolume", &lockOnSoundVolume, 0.5f, 0.01f, 0.0f, 2.0f);
    binder.Bind("ThrusterExplosionVolume", &thrusterExplosionVolume, 0.3f, 0.01f, 0.0f, 2.0f);

    binder.Bind("ThrusterOffset", &thrusterOffset, { 1.0f, 1.0f, 1.0f });
    binder.Bind("ThrusterScale", &thrusterScale, { 0.5f, 0.5f, 0.5f });
    binder.Bind("ThrusterColliderSize", &thrusterColliderSize, { 1.0f, 1.0f, 1.0f });
    binder.Bind("ThrusterHP", &thrusterMaxHP, 30, 1.0f, 1, 10000);
    binder.Bind("PropellerSpeed", &propellerSpeed, 40.0f, 0.5f, 0.0f, 200.0f);
    binder.Bind("BodyDamageRate", &bodyDamageRate, 0.3f, 0.01f, 0.0f, 1.0f);
    binder.Bind("ThrustersToCrash", &thrustersToCrash, 2, 1.0f, 1, 4);

    binder.Bind("CrashTime", &crashTime, 5.0f, 0.05f, 0.0f, 10.0f);
    binder.Bind("CrashGravity", &crashGravity, 30.0f, 0.5f, 0.0f, 200.0f);
    binder.Bind("DeathIgniteRadius", &deathIgniteRadius, 6.0f, 0.1f, 0.0f, 50.0f);
    binder.Bind("DeathIgniteGroundReach", &deathIgniteGroundReach, 5.0f, 0.1f, 0.0f, 100.0f);

    binder.BindColor("SpotColor", &spotColor, { 0.076f, 0.216f, 1.0f, 1.0f });
    binder.Bind("SpotIntensity", &spotIntensity, 30.85f);
    binder.Bind("SpotDistance", &spotDistance, 28.24f);
    binder.Bind("SpotAngle", &spotAngleDeg, 28.13f);
    binder.Bind("SpotVolumetric", &spotVolumetric, 200.0f);
    binder.Bind("SpotLocalDirection", &spotLocalDirection, { 0.0f, -0.3f, -1.0f });
    binder.BindColor("SpotAlertColor", &spotAlertColor, { 1.0f, 0.7f, 0.1f, 1.0f });
    binder.BindColor("SpotLockColor", &spotLockColor, { 1.0f, 0.08f, 0.05f, 1.0f });
    binder.Bind("SpotLockAngle", &spotLockAngleDeg, 6.0f, 0.1f, 0.5f, 90.0f);
    binder.Bind("SpotChangeSpeed", &spotChangeSpeed, 6.0f, 0.1f, 0.1f, 50.0f);
}

void FloatingEnemyConfig::Draw(PropertyBinder& binder)
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
        binder.Draw("IsSpawnEnabled", "出す");
        binder.Draw("MaxAliveCount", "フィールドにいる上限");
        binder.Draw("SpawnInterval", "減ってから次が飛んでくるまで (秒)");
        binder.Draw("SpawnMinDistance", "出す距離 (プレイヤーから、最小)");
        binder.Draw("SpawnMaxDistance", "出す距離 (プレイヤーから、最大)");
        binder.Draw("SpawnHeight", "出す高さ (地面から)");
        binder.Draw("InitialMinDistance", "遊び始めにいる機体をプレイヤーから離す距離");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("徘徊"))
    {
        binder.Draw("WanderSpeed", "速さ");
        binder.Draw("WanderHeight", "飛ぶ高さ (地面から)");
        binder.Draw("WanderMinStep", "次の目的地までの距離 (最小)");
        binder.Draw("WanderMaxStep", "次の目的地までの距離 (最大)");
        binder.Draw("WanderLeashDistance", "プレイヤーからこれより離れたら戻ってくる距離");
        binder.Draw("WanderPauseMinTime", "目的地で見回す時間 (最小、秒)");
        binder.Draw("WanderPauseMaxTime", "目的地で見回す時間 (最大、秒)");
        binder.Draw("ScanTurnSpeed", "見回す速さ (ラジアン/秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("発見"))
    {
        binder.Draw("SightRange", "気づく距離");
        binder.Draw("HearingRange", "銃声に気づく距離");
        binder.Draw("LoseRange", "見失う距離");
        binder.Draw("HiddenSightRange", "隠れているプレイヤーに気づく距離");
        binder.Draw("HiddenLoseTime", "隠れ続けると見失うまで (秒)");
        binder.Draw("ScreamVolume", "気づいたときの叫び声の大きさ");
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
        binder.Draw("MuzzleOffset", "銃口の位置 (機体のスケールが掛かる。-Z が正面)");
        binder.Draw("ShowMuzzleMarker", "銃口の位置に印を出す");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("狙い・撃ったとき"))
    {
        ImGui::SeparatorText("狙い");
        binder.Draw("AimTrackSharpness", "追いかける速さ (大きいほどぴったり付いてくる)");
        binder.Draw("LockFreezeTime", "撃つ前に狙いを止める時間 (秒)");
        ImGui::SeparatorText("ため (floatingCharge)");
        binder.Draw("ChargeRateStart", "狙い始めの吸い込む量 (倍)");
        binder.Draw("ChargeRateEnd", "撃つ直前の吸い込む量 (倍)");
        ImGui::SeparatorText("音 (置いたときだけ鳴る)");
        binder.Draw("ShotSoundVolume", "銃声 (floatingEnemyShot) の大きさ");
        binder.Draw("LockOnSoundVolume", "狙い始めの音 (floatingLockOn) の大きさ");
        binder.Draw("ThrusterExplosionVolume", "スラスターが壊れたときの爆発音 (floatingEnemyExplosion) の大きさ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("弱点・墜落"))
    {
        binder.Draw("ThrusterOffset", "スラスターの位置 (1 基目)");
        binder.Draw("ThrusterScale", "スラスターのスケール");
        binder.Draw("ThrusterColliderSize", "スラスターの当たり判定の大きさ");
        binder.Draw("ThrusterHP", "スラスター 1 基の HP");
        binder.Draw("PropellerSpeed", "プロペラの回る速さ (ラジアン / 秒)");
        binder.Draw("BodyDamageRate", "本体へのダメージ倍率");
        binder.Draw("ThrustersToCrash", "墜落するまでに壊す数");
        binder.Draw("CrashTime", "地面に届かないときに爆発するまで (秒)");
        binder.Draw("CrashGravity", "墜落の重力");
        binder.Draw("DeathIgniteRadius", "爆発した所の草に火がつく半径 (0 でつかない)");
        binder.Draw("DeathIgniteGroundReach", "火がつく、地面からの爆発の高さの上限");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("スポットライト"))
    {
        binder.Draw("SpotIntensity", "ライト輝度");
        binder.Draw("SpotDistance", "届く距離");
        binder.Draw("SpotAngle", "照射角");
        binder.Draw("SpotVolumetric", "ボリュームフォグ輝度");
        binder.Draw("SpotLocalDirection", "照射方向 (機体から見て。-Z が正面)");
        ImGui::SeparatorText("状態ごとの色");
        binder.Draw("SpotColor", "巡回中の色");
        binder.Draw("SpotAlertColor", "見つけて追いかけている間の色");
        binder.Draw("SpotLockColor", "狙っている間の色");
        binder.Draw("SpotLockAngle", "狙っている間の照射角 (絞る)");
        binder.Draw("SpotChangeSpeed", "色と絞りが移り変わる速さ");
        ImGui::TreePop();
    }
}
