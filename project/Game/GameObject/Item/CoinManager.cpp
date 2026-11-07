#include "pch.h"
#include "CoinManager.h"
#include "Player.h"
#include "Terrain.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "MathUtils.h"

using namespace FS;

namespace
{
    const Vector3 kUp = { 0.0f, 1.0f, 0.0f };
    constexpr float kTiltAngle = 0.35f;   // 面が光を受けるよう、少し傾けて回す (ラジアン)

    // 拾ったときの音。Assets/Audio に置けば鳴る
    const char* const kCollectSound = "coinCollect";
}

CoinManager::CoinManager(Engine* engine)
    : engine_(engine), random_(std::random_device{}())
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
    binder_->Bind("CoinsPerBurst", &c.coinsPerBurst, c.coinsPerBurst, 1.0f, 1, 100);
    binder_->Bind("ValuePerCoin", &c.valuePerCoin, c.valuePerCoin, 1.0f, 0, 10000);
    binder_->Bind("SpawnDuration", &c.spawnDuration, c.spawnDuration, 0.01f, 0.0f, 3.0f);
    binder_->Bind("EjectSpeed", &c.ejectSpeed, c.ejectSpeed, 0.1f, 0.0f, 100.0f);
    binder_->Bind("SpreadSpeed", &c.spreadSpeed, c.spreadSpeed, 0.1f, 0.0f, 100.0f);
    binder_->Bind("Gravity", &c.gravity, c.gravity, 0.5f, 0.0f, 200.0f);
    binder_->Bind("Bounce", &c.bounce, c.bounce, 0.01f, 0.0f, 1.0f);
    binder_->Bind("RestSpeed", &c.restSpeed, c.restSpeed, 0.05f, 0.0f, 20.0f);
    binder_->Bind("SpinSpeed", &c.spinSpeed, c.spinSpeed, 0.1f, 0.0f, 50.0f);
    binder_->Bind("MagnetDelay", &c.magnetDelay, c.magnetDelay, 0.01f, 0.0f, 5.0f);
    binder_->Bind("MagnetRadius", &c.magnetRadius, c.magnetRadius, 0.1f, 0.0f, 100.0f);
    binder_->Bind("MagnetAcceleration", &c.magnetAcceleration, c.magnetAcceleration, 1.0f, 0.0f, 1000.0f);
    binder_->Bind("MagnetMaxSpeed", &c.magnetMaxSpeed, c.magnetMaxSpeed, 0.5f, 0.0f, 500.0f);
    binder_->Bind("CollectRadius", &c.collectRadius, c.collectRadius, 0.05f, 0.0f, 10.0f);
    binder_->Bind("LifeTime", &c.lifeTime, c.lifeTime, 0.5f, 1.0f, 600.0f);
    binder_->Bind("BlinkTime", &c.blinkTime, c.blinkTime, 0.1f, 0.0f, 30.0f);
    binder_->Bind("Scale", &c.scale, c.scale, 0.005f);
    binder_->BindColor("LightColor", &c.lightColor, c.lightColor);
    binder_->Bind("LightIntensity", &c.lightIntensity, c.lightIntensity, 0.1f, 0.0f, 100.0f);
    binder_->Bind("LightRadius", &c.lightRadius, c.lightRadius, 0.1f, 0.0f, 100.0f);
    binder_->Bind("LightVolumetric", &c.lightVolumetric, c.lightVolumetric, 0.05f, 0.0f, 100.0f);
    binder_->Bind("LightHeight", &c.lightHeight, c.lightHeight, 0.05f, 0.0f, 10.0f);
    binder_->Bind("MaxLitBursts", &c.maxLitBursts, c.maxLitBursts, 1.0f, 0, 10);
}

void CoinManager::SpawnBurst(const Vector3& position)
{
    const int burstId = nextBurstId_++;
    for (int coinIndex = 0; coinIndex < config_.coinsPerBurst; ++coinIndex)
    {
        Coin coin;
        coin.model = std::make_unique<Model>(engine_, "cylinder");
        coin.model->ShareMaterialsFrom(sourceModel_.get());
        coin.position = position;
        coin.spawnDelay = config_.spawnDuration * static_cast<float>(coinIndex) / static_cast<float>(config_.coinsPerBurst);
        coin.spinAngle = std::uniform_real_distribution<float>(0.0f, Math::PI * 2.0f)(random_);
        coin.burstId = burstId;
        coins_.push_back(std::move(coin));
    }
}

void CoinManager::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;

    PlayerArea playerArea = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    if (player_)
    {
        playerArea = { player_->GetColliderCenter(), player_->GetColliderSize() };
    }
    const Vector3& playerCenter = playerArea.center;

    for (Coin& coin : coins_)
    {
        UpdateCoin(coin, deltaTime, playerArea);
    }
    std::erase_if(coins_, [this](const Coin& coin) {
        return coin.isCollected || coin.elapsedTime >= config_.lifeTime;
    });

    SubmitBurstLights(playerCenter);
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

    // 近くにいれば吸い寄せる。出た直後は散らばる様子を見せるため待つ
    // 体の大きさによらないよう、距離は当たり判定の箱の表面から測る
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

    case State::Resting:
        coin.spinAngle += config_.spinSpeed * deltaTime;
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
        coin.spinAngle += config_.spinSpeed * 4.0f * deltaTime;
        break;
    }

    default:
        break;
    }

    WorldTransform& transform = coin.model->GetTransform();
    transform.translation_ = coin.position;
    transform.SetRotation(Quaternion::FromAxisAngle(kUp, coin.spinAngle) * Quaternion::FromAxisAngle({ 1.0f, 0.0f, 0.0f }, kTiltAngle));
    transform.scale_ = config_.scale;
}

void CoinManager::UpdateFlying(Coin& coin, float deltaTime)
{
    coin.velocity.y -= config_.gravity * deltaTime;
    coin.position += coin.velocity * deltaTime;
    coin.spinAngle += config_.spinSpeed * 3.0f * deltaTime;

    float groundHeight = 0.0f;
    if (!terrain_ || !terrain_->GetHeightAt(coin.position.x, coin.position.z, groundHeight))
    {
        return;
    }

    // 少し浮かせて置く。円柱のモデルは高さ 1 なので、厚みの半分
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
    AudioPlayer::GetInstance().Play(kCollectSound, false, 30);
}

// 1 回分ずつ、残っている枚数の中心に点光源を置く。点光源の数には上限があるので、近いものだけ照らす
void CoinManager::SubmitBurstLights(const Vector3& playerCenter)
{
    struct BurstLight
    {
        Vector3 positionSum = { 0.0f, 0.0f, 0.0f };
        int count = 0;
    };
    std::map<int, BurstLight> bursts;
    for (const Coin& coin : coins_)
    {
        if (coin.state == State::Waiting || IsHiddenByBlink(coin))
        {
            continue;
        }
        BurstLight& burst = bursts[coin.burstId];
        burst.positionSum += coin.position;
        ++burst.count;
    }

    std::vector<std::pair<float, const BurstLight*>> sorted;
    for (const auto& [burstId, burst] : bursts)
    {
        const Vector3 center = burst.positionSum / static_cast<float>(burst.count);
        sorted.push_back({ (center - playerCenter).LengthSq(), &burst });
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    const int litCount = std::min(static_cast<int>(sorted.size()), config_.maxLitBursts);
    for (int lightIndex = 0; lightIndex < litCount; ++lightIndex)
    {
        const BurstLight& burst = *sorted[lightIndex].second;
        const Vector3 center = burst.positionSum / static_cast<float>(burst.count);
        const float remainingRatio = static_cast<float>(burst.count) / static_cast<float>(config_.coinsPerBurst);
        engine_->GetLightManager()->SubmitPointLight(
            center + kUp * config_.lightHeight,
            config_.lightColor,
            config_.lightIntensity * std::min(remainingRatio, 1.0f),
            config_.lightRadius,
            config_.lightVolumetric);
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
        binder_->Draw("CoinsPerBurst", "1 体から出る枚数");
        binder_->Draw("ValuePerCoin", "1 枚の金額");
        binder_->Draw("SpawnDuration", "出し切るまで (秒)");
        binder_->Draw("EjectSpeed", "上に飛び出す速さ");
        binder_->Draw("SpreadSpeed", "横に散らばる速さ");
        binder_->Draw("Gravity", "重力");
        binder_->Draw("Bounce", "跳ね返り");
        binder_->Draw("RestSpeed", "止まる速さ");
        binder_->Draw("SpinSpeed", "回る速さ");
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

    if (ImGui::TreeNode("見た目と光"))
    {
        binder_->Draw("Scale", "大きさ");
        binder_->Draw("LightColor", "光の色");
        binder_->Draw("LightIntensity", "光の強さ");
        binder_->Draw("LightRadius", "光の届く距離");
        binder_->Draw("LightVolumetric", "霧に当たる強さ");
        binder_->Draw("LightHeight", "光の高さ");
        binder_->Draw("MaxLitBursts", "同時に照らす数");
        binder_->DrawModel("CoinModel", "マテリアル (発光を上げるとブルームで光る)");
        ImGui::TreePop();
    }

    if (ImGui::Button("プレイヤーの前に出す") && player_)
    {
        SpawnBurst(player_->GetTransform().translation_ + Vector3(0.0f, 1.0f, 3.0f));
    }

    ImGui::End();
}
