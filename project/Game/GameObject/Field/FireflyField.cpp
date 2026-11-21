#include "pch.h"
#include "FireflyField.h"
#include "ImGuiManager.h"
#include "RendererManager.h"
#include "LightManager.h"
#include "EnvironmentManager.h"
#include "TextureManager.h"
#include "TimeManager.h"
#include "MathUtils.h"

using namespace FS;

namespace
{

constexpr float kTwoPi = 6.2831853f;

// 整数から乱れた整数を作る
uint32_t HashUint(uint32_t value)
{
    const uint32_t state = value * 747796405u + 2891336453u;
    const uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// マスの座標と番号から、いつも同じ 0 ~ 1 の値を作る
float CalculateCellRandom(int cellX, int cellZ, uint32_t index, uint32_t salt)
{
    uint32_t hash = HashUint(static_cast<uint32_t>(cellX) ^ HashUint(salt));
    hash = HashUint(hash ^ static_cast<uint32_t>(cellZ));
    hash = HashUint(hash ^ index);
    return static_cast<float>(hash & 0x00FFFFFFu) / 16777216.0f;
}

float SmoothStep(float edge0, float edge1, float value)
{
    const float ratio = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return ratio * ratio * (3.0f - 2.0f * ratio);
}

// 乱数の使い道ごとに混ぜる値
enum RandomSalt : uint32_t
{
    kSaltClusterExists = 1,
    kSaltClusterPositionX,
    kSaltClusterPositionZ,
    kSaltClusterRadius,
    kSaltClusterPhase,
    kSaltCount,
    kSaltPositionX,
    kSaltPositionZ,
    kSaltHeight,
    kSaltDrift,
    kSaltBlinkPeriod,
    kSaltBlinkPhase,
};

}

FireflyField::FireflyField(Engine* engine, const Terrain* terrain, const WorldTransform* playerTransform, const std::string& groupName)
    : GameObject(), engine_(engine), terrain_(terrain), playerTransform_(playerTransform)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, groupName);
}

void FireflyField::Initialize()
{
    textureHandle_ = TextureManager::GetInstance().Get("circle_05");

    binder_->Bind("CellSize", &cellSize_, 4.0f, 0.1f, 1.0f, 20.0f);
    binder_->Bind("BaseDensity", &baseDensity_, 0.004f, 0.001f, 0.0f, 1.0f);
    binder_->Bind("ClusterDensity", &clusterDensity_, 0.12f, 0.005f, 0.0f, 2.0f);
    binder_->Bind("ClusterCellSize", &clusterCellSize_, 40.0f, 1.0f, 10.0f, 300.0f);
    binder_->Bind("ClusterProbability", &clusterProbability_, 0.35f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("ClusterRadiusMin", &clusterRadiusMin_, 5.0f, 0.5f, 1.0f, 50.0f);
    binder_->Bind("ClusterRadiusMax", &clusterRadiusMax_, 12.0f, 0.5f, 1.0f, 50.0f);
    binder_->Bind("HeightMin", &heightMin_, 0.3f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("HeightMax", &heightMax_, 2.0f, 0.05f, 0.0f, 10.0f);

    binder_->Bind("ActiveRadius", &activeRadius_, 30.0f, 0.5f, 5.0f, 100.0f);
    binder_->Bind("FadeStartRatio", &fadeStartRatio_, 0.6f, 0.01f, 0.0f, 0.99f);
    binder_->Bind("MaxCount", &maxCount_, 200, 1.0f, 0, 2000);

    binder_->Bind("DriftDistance", &driftDistance_, 1.2f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("DriftHeight", &driftHeight_, 0.4f, 0.05f, 0.0f, 5.0f);
    binder_->Bind("DriftSpeed", &driftSpeed_, 0.12f, 0.01f, 0.0f, 2.0f);

    binder_->Bind("BlinkPeriodMin", &blinkPeriodMin_, 2.0f, 0.05f, 0.1f, 20.0f);
    binder_->Bind("BlinkPeriodMax", &blinkPeriodMax_, 5.0f, 0.05f, 0.1f, 20.0f);
    binder_->Bind("GlowDuration", &glowDuration_, 0.8f, 0.05f, 0.05f, 10.0f);
    binder_->Bind("DimBrightness", &dimBrightness_, 0.05f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("ClusterSync", &clusterSync_, 0.6f, 0.01f, 0.0f, 1.0f);

    binder_->BindColor("Color", &color_, { 0.75f, 1.0f, 0.3f, 1.0f });
    binder_->Bind("Size", &size_, 0.08f, 0.005f, 0.005f, 1.0f);
    binder_->Bind("Intensity", &intensity_, 8.0f, 0.1f, 0.0f, 100.0f);

    binder_->Bind("NightStartSunHeight", &nightStartSunHeight_, 0.05f, 0.01f, -1.0f, 1.0f);
    binder_->Bind("NightFullSunHeight", &nightFullSunHeight_, -0.1f, 0.01f, -1.0f, 1.0f);
    binder_->Bind("RainCutoff", &rainCutoff_, 0.2f, 0.01f, 0.0f, 1.0f);

    binder_->Bind("MaxLightCount", &maxLightCount_, 2, 1.0f, 0, 8);
    binder_->Bind("LightDistance", &lightDistance_, 25.0f, 0.5f, 0.0f, 100.0f);
    binder_->BindColor("LightColor", &lightColor_, { 0.7f, 1.0f, 0.35f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 1.5f, 0.05f, 0.0f, 50.0f);
    binder_->Bind("LightRadius", &lightRadius_, 8.0f, 0.5f, 0.5f, 50.0f);
    binder_->Bind("LightHeight", &lightHeight_, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("LightVolumetricIntensity", &lightVolumetricIntensity_, 1.0f, 0.05f, 0.0f, 20.0f);
}

void FireflyField::Update()
{
    elapsedTime_ += TimeManager::GetInstance()->GetDeltaTime();
    submittedCount_ = 0;
    submittedLightCount_ = 0;

    const float activity = CalculateActivity();
    if (!playerTransform_ || !terrain_ || activity <= 0.001f)
    {
        return;
    }

    const Vector3 playerPosition = playerTransform_->translation_;
    CollectClusters(playerPosition);
    SubmitFireflies(playerPosition, activity);
    SubmitClusterLights(playerPosition, activity);
}

float FireflyField::CalculateActivity() const
{
    EnvironmentManager* environment = EnvironmentManager::GetInstance();

    // 光の向きは太陽から向かう向きなので、太陽の高さは符号を逆にする
    const float sunHeight = -environment->GetSunDirection().y;
    const float night = SmoothStep(nightStartSunHeight_, nightFullSunHeight_, sunHeight);

    const float rain = environment->GetCurrentWeatherProfile().rainIntensity;
    const float dry = (rainCutoff_ > 0.0f) ? 1.0f - SmoothStep(0.0f, rainCutoff_, rain) : 1.0f;
    return night * dry;
}

void FireflyField::CollectClusters(const Vector3& playerPosition)
{
    clusters_.clear();
    if (clusterCellSize_ <= 0.0f || clusterProbability_ <= 0.0f)
    {
        return;
    }

    // 動かす範囲に少しでもかかる集まりを、周りの粗いマスから探す
    const float reach = std::max(activeRadius_, lightDistance_) + clusterRadiusMax_;
    const int minX = static_cast<int>(std::floor((playerPosition.x - reach) / clusterCellSize_));
    const int maxX = static_cast<int>(std::floor((playerPosition.x + reach) / clusterCellSize_));
    const int minZ = static_cast<int>(std::floor((playerPosition.z - reach) / clusterCellSize_));
    const int maxZ = static_cast<int>(std::floor((playerPosition.z + reach) / clusterCellSize_));

    for (int cellZ = minZ; cellZ <= maxZ; ++cellZ)
    {
        for (int cellX = minX; cellX <= maxX; ++cellX)
        {
            if (CalculateCellRandom(cellX, cellZ, 0, kSaltClusterExists) >= clusterProbability_)
            {
                continue;
            }

            // 粗いマスの端に寄せず、隣の集まりと重なりにくくする
            Cluster cluster;
            cluster.center.x = (static_cast<float>(cellX) + 0.2f + 0.6f * CalculateCellRandom(cellX, cellZ, 0, kSaltClusterPositionX)) * clusterCellSize_;
            cluster.center.z = (static_cast<float>(cellZ) + 0.2f + 0.6f * CalculateCellRandom(cellX, cellZ, 0, kSaltClusterPositionZ)) * clusterCellSize_;
            if (!terrain_->GetHeightAt(cluster.center.x, cluster.center.z, cluster.center.y))
            {
                continue;
            }
            cluster.radius = clusterRadiusMin_ + (clusterRadiusMax_ - clusterRadiusMin_) * CalculateCellRandom(cellX, cellZ, 0, kSaltClusterRadius);
            cluster.blinkPhase = CalculateCellRandom(cellX, cellZ, 0, kSaltClusterPhase);
            clusters_.push_back(cluster);
        }
    }
}

void FireflyField::RebuildCellOffsets()
{
    builtActiveRadius_ = activeRadius_;
    builtCellSize_ = cellSize_;
    cellOffsets_.clear();

    // マスの中心がどこにあっても範囲を覆えるよう、1 マス分広く集める
    const int range = static_cast<int>(std::ceil(activeRadius_ / cellSize_)) + 1;
    for (int offsetZ = -range; offsetZ <= range; ++offsetZ)
    {
        for (int offsetX = -range; offsetX <= range; ++offsetX)
        {
            cellOffsets_.emplace_back(offsetX, offsetZ);
        }
    }

    // 上限に届いたときに遠くから削れるよう、近い順に並べる
    std::sort(cellOffsets_.begin(), cellOffsets_.end(), [](const auto& a, const auto& b)
        {
            return a.first * a.first + a.second * a.second < b.first * b.first + b.second * b.second;
        });
}

void FireflyField::SubmitFireflies(const Vector3& playerPosition, float activity)
{
    if (cellSize_ <= 0.0f || maxCount_ <= 0)
    {
        return;
    }
    if (builtActiveRadius_ != activeRadius_ || builtCellSize_ != cellSize_)
    {
        RebuildCellOffsets();
    }

    RendererManager* rendererManager = engine_->GetRendererManager();
    const int playerCellX = static_cast<int>(std::floor(playerPosition.x / cellSize_));
    const int playerCellZ = static_cast<int>(std::floor(playerPosition.z / cellSize_));
    const float cellArea = cellSize_ * cellSize_;
    const float cellReach = activeRadius_ + cellSize_ * 0.75f;
    const float fadeStart = activeRadius_ * fadeStartRatio_;
    const float driftPhase = elapsedTime_ * driftSpeed_ * kTwoPi;

    WorldTransform transform;
    transform.scale_ = { size_, size_, 1.0f };

    for (const auto& [offsetX, offsetZ] : cellOffsets_)
    {
        const int cellX = playerCellX + offsetX;
        const int cellZ = playerCellZ + offsetZ;
        const float cellCenterX = (static_cast<float>(cellX) + 0.5f) * cellSize_;
        const float cellCenterZ = (static_cast<float>(cellZ) + 0.5f) * cellSize_;
        const float cellOffsetX = cellCenterX - playerPosition.x;
        const float cellOffsetZ = cellCenterZ - playerPosition.z;
        if (cellOffsetX * cellOffsetX + cellOffsetZ * cellOffsetZ > cellReach * cellReach)
        {
            continue;
        }

        // 一番近い集まりほど数を増やし、明滅のタイミングをその集まりにそろえる
        float clusterWeight = 0.0f;
        float clusterPhase = 0.0f;
        for (const Cluster& cluster : clusters_)
        {
            const float distanceX = cellCenterX - cluster.center.x;
            const float distanceZ = cellCenterZ - cluster.center.z;
            const float normalizedDistanceSq = (distanceX * distanceX + distanceZ * distanceZ) / (cluster.radius * cluster.radius);
            const float weight = std::exp(-2.0f * normalizedDistanceSq);
            if (weight > clusterWeight)
            {
                clusterWeight = weight;
                clusterPhase = cluster.blinkPhase;
            }
        }

        // 数は小数になるので、余りの分は乱数で 1 匹増やすかを決める
        const float expectedCount = (baseDensity_ + clusterDensity_ * clusterWeight) * cellArea;
        const float extraRandom = CalculateCellRandom(cellX, cellZ, 0, kSaltCount);
        const int count = std::min(static_cast<int>(expectedCount) + ((extraRandom < expectedCount - std::floor(expectedCount)) ? 1 : 0), 32);

        for (int index = 0; index < count; ++index)
        {
            if (submittedCount_ >= maxCount_)
            {
                return;
            }

            const uint32_t fireflyIndex = static_cast<uint32_t>(index) + 1u;
            const float homeX = (static_cast<float>(cellX) + CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltPositionX)) * cellSize_;
            const float homeZ = (static_cast<float>(cellZ) + CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltPositionZ)) * cellSize_;
            float groundHeight = 0.0f;
            if (!terrain_->GetHeightAt(homeX, homeZ, groundHeight))
            {
                continue;
            }

            // 速さの違う揺れを重ね、まっすぐ飛ばずにゆっくり漂わせる
            const float driftRandom = CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltDrift);
            const float phase = driftRandom * kTwoPi;
            const float speed = 0.7f + driftRandom * 0.6f;
            const float swayX = std::sin(driftPhase * speed + phase) + 0.4f * std::sin(driftPhase * speed * 2.7f + phase * 3.1f);
            const float swayZ = std::cos(driftPhase * speed * 0.8f + phase * 1.7f) + 0.4f * std::sin(driftPhase * speed * 2.3f + phase * 5.3f);
            const float swayY = std::sin(driftPhase * speed * 1.3f + phase * 2.3f);

            const float heightRatio = CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltHeight);
            Vector3 position = {
                homeX + swayX * driftDistance_ / 1.4f,
                groundHeight + heightMin_ + (heightMax_ - heightMin_) * heightRatio + swayY * driftHeight_,
                homeZ + swayZ * driftDistance_ / 1.4f };

            // 動かす範囲のふちに近いほど薄くし、出入りを見せない
            const Vector3 toFirefly = position - playerPosition;
            const float distance = std::sqrt(toFirefly.x * toFirefly.x + toFirefly.y * toFirefly.y + toFirefly.z * toFirefly.z);
            const float distanceFade = 1.0f - SmoothStep(fadeStart, activeRadius_, distance);
            if (distanceFade <= 0.0f)
            {
                continue;
            }

            // ほとんどの時間は暗く、周期のはじめに glowDuration_ 秒だけ光る
            const float period = blinkPeriodMin_ + (blinkPeriodMax_ - blinkPeriodMin_) * CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltBlinkPeriod);
            const float ownPhase = CalculateCellRandom(cellX, cellZ, fireflyIndex, kSaltBlinkPhase);
            const float blinkPhase = ownPhase + (clusterPhase - ownPhase) * clusterSync_ * clusterWeight;
            const float cycle = elapsedTime_ / std::max(period, 0.01f) + blinkPhase;
            const float glowTime = (cycle - std::floor(cycle)) * period;
            const float halfGlow = std::max(glowDuration_ * 0.5f, 0.01f);
            const float pulse = SmoothStep(0.0f, 1.0f, 1.0f - std::abs(glowTime - halfGlow) / halfGlow);
            const float brightness = dimBrightness_ + (1.0f - dimBrightness_) * pulse;

            const float alpha = brightness * distanceFade * activity;
            if (alpha < 0.01f)
            {
                continue;
            }

            transform.translation_ = position;
            const Vector4 color = { color_.x, color_.y, color_.z, std::min(alpha, 1.0f) };
            rendererManager->SubmitParticleInstance(transform, Math::ColorVectorToUint32(color), textureHandle_, 0.0f,
                kBlendModeAdd, true, intensity_);
            ++submittedCount_;
        }
    }
}

void FireflyField::SubmitClusterLights(const Vector3& playerPosition, float activity)
{
    if (maxLightCount_ <= 0 || lightDistance_ <= 0.0f)
    {
        return;
    }

    // 近い集まりから順にライトを置く
    std::vector<std::pair<float, const Cluster*>> nearClusters;
    for (const Cluster& cluster : clusters_)
    {
        const float distanceX = cluster.center.x - playerPosition.x;
        const float distanceZ = cluster.center.z - playerPosition.z;
        const float distance = std::sqrt(distanceX * distanceX + distanceZ * distanceZ);
        if (distance < lightDistance_)
        {
            nearClusters.emplace_back(distance, &cluster);
        }
    }
    std::sort(nearClusters.begin(), nearClusters.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    LightManager* lightManager = engine_->GetLightManager();
    const size_t lightCount = std::min(nearClusters.size(), static_cast<size_t>(maxLightCount_));
    for (size_t index = 0; index < lightCount; ++index)
    {
        const auto& [distance, cluster] = nearClusters[index];

        // 近づくにつれてゆっくり明るくし、集まりの明滅に合わせて少し揺らす
        const float distanceFade = 1.0f - SmoothStep(lightDistance_ * 0.6f, lightDistance_, distance);
        const float pulse = 0.8f + 0.2f * std::sin((elapsedTime_ * 0.8f + cluster->blinkPhase) * kTwoPi);

        lightManager->SubmitPointLight(
            cluster->center + Vector3(0.0f, lightHeight_, 0.0f),
            lightColor_,
            lightIntensity_ * distanceFade * activity * pulse,
            lightRadius_,
            lightVolumetricIntensity_);
        ++submittedLightCount_;
    }
}

void FireflyField::DebugGui()
{
    ImGui::Begin("蛍");

    ImGui::Text("出る量: %.2f / 描いている数: %d / ライト: %d / 近くの集まり: %d",
        CalculateActivity(), submittedCount_, submittedLightCount_, static_cast<int>(clusters_.size()));

    ImGui::SeparatorText("居場所と数");
    binder_->Draw("CellSize", "居場所を決めるマスの大きさ (m)");
    binder_->Draw("BaseDensity", "集まりの外の数 (1 m² あたり)");
    binder_->Draw("ClusterDensity", "集まりの真ん中の数 (1 m² あたり)");
    binder_->Draw("ClusterCellSize", "集まりを置くか決めるマスの大きさ (m)");
    binder_->Draw("ClusterProbability", "集まりを置く割合");
    binder_->Draw("ClusterRadiusMin", "集まりの半径の最小 (m)");
    binder_->Draw("ClusterRadiusMax", "集まりの半径の最大 (m)");
    binder_->Draw("HeightMin", "地面からの高さの最小 (m)");
    binder_->Draw("HeightMax", "地面からの高さの最大 (m)");

    ImGui::SeparatorText("近くだけ動かす範囲");
    binder_->Draw("ActiveRadius", "プレイヤーからの半径 (m)");
    binder_->Draw("FadeStartRatio", "薄くし始める割合");
    binder_->Draw("MaxCount", "同時に描く数の上限");

    ImGui::SeparatorText("漂う動き");
    binder_->Draw("DriftDistance", "横に漂う大きさ (m)");
    binder_->Draw("DriftHeight", "上下に漂う大きさ (m)");
    binder_->Draw("DriftSpeed", "漂う速さ");

    ImGui::SeparatorText("明滅");
    binder_->Draw("BlinkPeriodMin", "周期の最短 (秒)");
    binder_->Draw("BlinkPeriodMax", "周期の最長 (秒)");
    binder_->Draw("GlowDuration", "光っている長さ (秒)");
    binder_->Draw("DimBrightness", "光っていない間の明るさ");
    binder_->Draw("ClusterSync", "集まりの中でそろえる強さ");

    ImGui::SeparatorText("見た目");
    binder_->Draw("Color", "色");
    binder_->Draw("Size", "大きさ (m)");
    binder_->Draw("Intensity", "発光の強さ");

    ImGui::SeparatorText("出る時間と天気");
    binder_->Draw("NightStartSunHeight", "出始める太陽の高さ");
    binder_->Draw("NightFullSunHeight", "一番多くなる太陽の高さ");
    binder_->Draw("RainCutoff", "出なくなる雨の強さ");

    ImGui::SeparatorText("集まりのライト (近づいたときだけ)");
    binder_->Draw("MaxLightCount", "ライトの数");
    binder_->Draw("LightDistance", "ライトを置く距離 (m)");
    binder_->Draw("LightColor", "色");
    binder_->Draw("LightIntensity", "強さ");
    binder_->Draw("LightRadius", "届く距離 (m)");
    binder_->Draw("LightHeight", "地面からの高さ (m)");
    binder_->Draw("LightVolumetricIntensity", "霧の中の光の筋の強さ");

    ImGui::End();
}
