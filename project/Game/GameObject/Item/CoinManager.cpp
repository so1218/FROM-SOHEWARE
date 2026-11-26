#include "pch.h"
#include "CoinManager.h"
#include "Player.h"
#include "Terrain.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "MathUtils.h"

using namespace FS;

CoinManager::CoinManager(Engine* engine, Player* player, const Terrain* terrain)
    : engine_(engine), player_(player), terrain_(terrain), random_(std::random_device{}())
{
    sourceModel_ = std::make_unique<Model>(engine_, "cylinder");
    sourceModel_->MakeMaterialUnique();
}

CoinManager::~CoinManager() = default;

void CoinManager::Initialize()
{
    // シーンに入り直したときは前回の分を消す
    coins_.clear();
    nextBurstId_ = 0;

    binder_ = std::make_unique<PropertyBinder>(engine_, "Coin");
    binder_->BindModel("CoinModel", sourceModel_.get());

    Config& c = config_;
    binder_->Bind("ZombieCoinCount", &c.zombieCoinCount, c.zombieCoinCount, 1.0f, 0, 100);
    binder_->Bind("FloatingCoinCount", &c.floatingCoinCount, c.floatingCoinCount, 1.0f, 0, 100);
    binder_->Bind("ValuePerCoin", &c.valuePerCoin, c.valuePerCoin, 1.0f, 0, 10000);
    binder_->Bind("SpawnDuration", &c.spawnDuration, c.spawnDuration, 0.01f, 0.0f, 3.0f);
    binder_->Bind("BurstSoundVolume", &c.burstSoundVolume, c.burstSoundVolume, 0.01f, 0.0f, 2.0f);
    binder_->Bind("EjectSpeed", &c.ejectSpeed, c.ejectSpeed, 0.1f, 0.0f, 100.0f);
    binder_->Bind("SpreadSpeed", &c.spreadSpeed, c.spreadSpeed, 0.1f, 0.0f, 100.0f);
    binder_->Bind("Gravity", &c.gravity, c.gravity, 0.5f, 0.0f, 200.0f);
    binder_->Bind("Bounce", &c.bounce, c.bounce, 0.01f, 0.0f, 1.0f);
    binder_->Bind("RestSpeed", &c.restSpeed, c.restSpeed, 0.05f, 0.0f, 20.0f);
    binder_->Bind("MagnetDelay", &c.magnetDelay, c.magnetDelay, 0.01f, 0.0f, 5.0f);
    binder_->Bind("MagnetRadius", &c.magnetRadius, c.magnetRadius, 0.1f, 0.0f, 100.0f);
    binder_->Bind("MagnetAcceleration", &c.magnetAcceleration, c.magnetAcceleration, 1.0f, 0.0f, 1000.0f);
    binder_->Bind("MagnetMaxSpeed", &c.magnetMaxSpeed, c.magnetMaxSpeed, 0.5f, 0.0f, 500.0f);
    binder_->Bind("CollectRadius", &c.collectRadius, c.collectRadius, 0.05f, 0.0f, 10.0f);
    binder_->Bind("LifeTime", &c.lifeTime, c.lifeTime, 0.5f, 1.0f, 600.0f);
    binder_->Bind("BlinkTime", &c.blinkTime, c.blinkTime, 0.1f, 0.0f, 30.0f);
    binder_->Bind("Scale", &c.scale, c.scale, 0.005f);
    binder_->Bind("MaxSparkleBursts", &c.maxSparkleBursts, c.maxSparkleBursts, 1.0f, 0, 16);
    binder_->Bind("SparkleHeight", &c.sparkleHeight, c.sparkleHeight, 0.01f, -5.0f, 10.0f);

    sparkleEmitters_.clear();
    isSparklePlaying_.clear();
}

void CoinManager::SpawnBurst(const Vector3& position, int coinCount)
{
    if (coinCount <= 0)
    {
        return;
    }
    const int burstId = nextBurstId_++;

    // 1 枚ずつ鳴らすと重なって割れるので、まとめて散らばる音を 1 回だけ鳴らす
    AudioPlayer::GetInstance().Play("coinBurst", config_.burstSoundVolume);

    for (int coinIndex = 0; coinIndex < coinCount; ++coinIndex)
    {
        Coin coin;
        coin.model = std::make_unique<Model>(engine_, "cylinder");
        coin.model->ShareMaterialsFrom(sourceModel_.get());
        coin.position = position;
        coin.spawnDelay = config_.spawnDuration * static_cast<float>(coinIndex) / static_cast<float>(coinCount);
        coin.burstId = burstId;
        coins_.push_back(std::move(coin));
    }
}

void CoinManager::Clear()
{
    coins_.clear();
    StopSparkles();
}

void CoinManager::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;

    PlayerArea playerArea = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    if (player_)
    {
        playerArea = { player_->GetColliderCenter(), player_->GetColliderHalfSize() };
    }
    const Vector3& playerCenter = playerArea.center;

    for (Coin& coin : coins_)
    {
        UpdateCoin(coin, deltaTime, playerArea);
    }
    std::erase_if(coins_, [this](const Coin& coin) {
        return coin.isCollected || coin.elapsedTime >= config_.lifeTime;
    });

    UpdateSparkles(playerCenter);
}

void CoinManager::UpdateCoin(Coin& coin, float deltaTime, const PlayerArea& playerArea)
{
    if (coin.state == State::Waiting)
    {
        coin.spawnDelay -= deltaTime;
        if (coin.spawnDelay > 0.0f)
        {
            return;
        }

        // 上に飛び出しつつ、横にばらける
        std::uniform_real_distribution<float> spread(-config_.spreadSpeed, config_.spreadSpeed);
        coin.velocity = { spread(random_), config_.ejectSpeed, spread(random_) };
        coin.state = State::Flying;
    }

    coin.elapsedTime += deltaTime;

    // 近くにいれば吸い寄せる
    const bool canAttract = player_ && player_->IsControllable() && coin.elapsedTime >= config_.magnetDelay;
    if (coin.state != State::Attracted && canAttract)
    {
        const Vector3 offset = coin.position - playerArea.center;
        const Vector3 outside = {
            std::max(std::abs(offset.x) - playerArea.halfSize.x, 0.0f),
            std::max(std::abs(offset.y) - playerArea.halfSize.y, 0.0f),
            std::max(std::abs(offset.z) - playerArea.halfSize.z, 0.0f)
        };
        if (outside.Length() <= config_.magnetRadius)
        {
            coin.state = State::Attracted;
        }
    }

    switch (coin.state)
    {
    case State::Flying:
        UpdateFlying(coin, deltaTime);
        break;

    case State::Attracted:
    {
        // 横向きの勢いを残すとプレイヤーの周りを回り続けるので、まっすぐ向かわせる
        const Vector3 toPlayer = playerArea.center - coin.position;
        const float distance = toPlayer.Length();
        coin.attractSpeed = std::min(coin.attractSpeed + config_.magnetAcceleration * deltaTime, config_.magnetMaxSpeed);
        const float step = coin.attractSpeed * deltaTime;

        if (IsInsidePlayer(coin.position, playerArea) || step >= distance)
        {
            Collect(coin);
            return;
        }
        coin.position += toPlayer / distance * step;
        break;
    }

    default:
        break;
    }

    WorldTransform& transform = coin.model->GetTransform();
    transform.translation_ = coin.position;
    transform.scale_ = config_.scale;
}

void CoinManager::UpdateFlying(Coin& coin, float deltaTime)
{
    coin.velocity.y -= config_.gravity * deltaTime;
    coin.position += coin.velocity * deltaTime;

    float groundHeight = 0.0f;
    if (!terrain_ || !terrain_->GetHeightAt(coin.position.x, coin.position.z, groundHeight))
    {
        return;
    }

    // 少し浮かせて置く
    const float restHeight = groundHeight + config_.scale.y * 0.5f;
    if (coin.position.y > restHeight)
    {
        return;
    }
    coin.position.y = restHeight;

    if (-coin.velocity.y < config_.restSpeed)
    {
        coin.velocity = { 0.0f, 0.0f, 0.0f };
        coin.state = State::Resting;
        return;
    }

    coin.velocity.y = -coin.velocity.y * config_.bounce;
    coin.velocity.x *= 0.6f;
    coin.velocity.z *= 0.6f;
}

// 当たり判定の箱を collectRadius だけ広げた中に入ったら拾う
bool CoinManager::IsInsidePlayer(const Vector3& position, const PlayerArea& playerArea) const
{
    const Vector3 offset = position - playerArea.center;
    return std::abs(offset.x) <= playerArea.halfSize.x + config_.collectRadius
        && std::abs(offset.y) <= playerArea.halfSize.y + config_.collectRadius
        && std::abs(offset.z) <= playerArea.halfSize.z + config_.collectRadius;
}

void CoinManager::Collect(Coin& coin)
{
    coin.isCollected = true;
    player_->AddMoney(config_.valuePerCoin);
    AudioPlayer::GetInstance().Play("coinCollect", 0.3f);
}

// 落ちているアイテムが光の粒を立ち昇らせて「拾える」と知らせる (FF7 リメイクのアイテムと同じ考え方)
void CoinManager::UpdateSparkles(const Vector3& playerCenter)
{
    // 地面に落ちて止まっているコインだけを、散らばった 1 回分ごとにまとめる
    struct Burst
    {
        Vector3 positionSum = { 0.0f, 0.0f, 0.0f };
        int count = 0;
    };
    std::map<int, Burst> bursts;
    for (const Coin& coin : coins_)
    {
        if (coin.state != State::Resting)
        {
            continue;
        }
        Burst& burst = bursts[coin.burstId];
        burst.positionSum += coin.position;
        ++burst.count;
    }

    std::vector<std::pair<float, Vector3>> centers;
    for (const auto& [burstId, burst] : bursts)
    {
        const Vector3 center = burst.positionSum / static_cast<float>(burst.count);
        centers.push_back({ (center - playerCenter).LengthSq(), center });
    }
    std::sort(centers.begin(), centers.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    // 足りない分だけ作り、作ったものは止めて使い回す
    const size_t emitterCount = static_cast<size_t>(std::max(config_.maxSparkleBursts, 0));
    while (sparkleEmitters_.size() < emitterCount)
    {
        sparkleEmitters_.push_back(engine_->GetParticleSystem()->SpawnEmitter("coinSparkle"));
        isSparklePlaying_.push_back(false);
    }

    for (size_t index = 0; index < sparkleEmitters_.size(); ++index)
    {
        ParticleEmitter* emitter = sparkleEmitters_[index];
        if (!emitter)
        {
            continue;
        }

        const bool shouldPlay = index < emitterCount && index < centers.size();
        if (shouldPlay)
        {
            emitter->SetPosition(centers[index].second + Vector3(0.0f, config_.sparkleHeight, 0.0f));
        }
        if (shouldPlay != isSparklePlaying_[index])
        {
            shouldPlay ? emitter->Play() : emitter->Stop();
            isSparklePlaying_[index] = shouldPlay;
        }
    }
}

void CoinManager::StopSparkles()
{
    for (size_t index = 0; index < sparkleEmitters_.size(); ++index)
    {
        if (sparkleEmitters_[index] && isSparklePlaying_[index])
        {
            sparkleEmitters_[index]->Stop();
        }
        isSparklePlaying_[index] = false;
    }
}

// 消える前の数秒は点滅させる
bool CoinManager::IsHiddenByBlink(const Coin& coin) const
{
    const float remainingTime = config_.lifeTime - coin.elapsedTime;
    if (remainingTime > config_.blinkTime)
    {
        return false;
    }
    return std::fmod(elapsedTime_ * 8.0f, 1.0f) < 0.5f;
}

void CoinManager::Draw()
{
    for (const Coin& coin : coins_)
    {
        if (coin.state == State::Waiting || IsHiddenByBlink(coin))
        {
            continue;
        }
        coin.model->Draw();
    }
}

void CoinManager::DebugGui()
{
    ImGui::Begin("お金");
    ImGui::Text("落ちている数: %d", static_cast<int>(coins_.size()));

    if (ImGui::TreeNode("出る量と飛び散り方"))
    {
        binder_->Draw("ZombieCoinCount", "ゾンビから出る枚数");
        binder_->Draw("FloatingCoinCount", "floating から出る枚数");
        binder_->Draw("ValuePerCoin", "1 枚の金額");
        binder_->Draw("SpawnDuration", "出し切るまで (秒)");
        binder_->Draw("BurstSoundVolume", "飛び出すときの音 (coinBurst) の大きさ");
        binder_->Draw("EjectSpeed", "上に飛び出す速さ");
        binder_->Draw("SpreadSpeed", "横に散らばる速さ");
        binder_->Draw("Gravity", "重力");
        binder_->Draw("Bounce", "跳ね返り");
        binder_->Draw("RestSpeed", "止まる速さ");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("拾い方"))
    {
        binder_->Draw("MagnetDelay", "吸い寄せ始めるまで (秒)");
        binder_->Draw("MagnetRadius", "吸い寄せる距離 (当たり判定の表面から)");
        binder_->Draw("MagnetAcceleration", "吸い寄せる加速");
        binder_->Draw("MagnetMaxSpeed", "吸い寄せる最高速度");
        binder_->Draw("CollectRadius", "拾う範囲 (当たり判定の外側にどれだけ広げるか)");
        binder_->Draw("LifeTime", "消えるまで (秒)");
        binder_->Draw("BlinkTime", "点滅する時間 (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("見た目ときらめき"))
    {
        binder_->Draw("Scale", "大きさ");
        binder_->Draw("MaxSparkleBursts", "きらめきを出す数 (プレイヤーに近い順)");
        binder_->Draw("SparkleHeight", "きらめきを出す高さ");
        binder_->DrawModel("CoinModel", "マテリアル");
        ImGui::TreePop();
    }

    if (ImGui::Button("プレイヤーの前に出す") && player_)
    {
        SpawnBurst(player_->GetTransform().translation_ + Vector3(0.0f, 1.0f, 3.0f), config_.zombieCoinCount);
    }

    ImGui::End();
}
