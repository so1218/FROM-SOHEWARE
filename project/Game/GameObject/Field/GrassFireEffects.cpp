#include "pch.h"
#include "GrassFireEffects.h"
#include "ImGuiManager.h"
#include "RendererManager.h"
#include "LightManager.h"
#include "TimeManager.h"
#include "MathUtils.h"
#include "AudioPlayer.h"

using namespace FS;

GrassFireEffects::GrassFireEffects(Engine* engine, const Terrain* terrain)
    : GameObject(), engine_(engine), terrain_(terrain)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "GrassFireEffects");
}

GrassFireEffects::~GrassFireEffects()
{
    StopFireSounds();
}

void GrassFireEffects::Initialize()
{
    binder_->Bind("PeakThreshold", &peakThreshold_, 0.12f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("PeakSeparation", &peakSeparation_, 10.0f, 0.5f, 1.0f, 100.0f);

    binder_->Bind("MaxLightCount", &maxLightCount_, 6, 1.0f, 0, 16);
    binder_->BindColor("LightColor", &lightColor_, { 1.0f, 0.45f, 0.12f, 1.0f });
    binder_->Bind("LightIntensity", &lightIntensity_, 12.0f, 0.1f, 0.0f, 200.0f);
    binder_->Bind("LightRadius", &lightRadius_, 14.0f, 0.5f, 1.0f, 100.0f);
    binder_->Bind("LightHeight", &lightHeight_, 1.5f, 0.1f, 0.0f, 20.0f);
    binder_->Bind("LightVolumetricIntensity", &lightVolumetricIntensity_, 2.0f, 0.1f, 0.0f, 50.0f);

    binder_->Bind("MaxEmberEmitterCount", &maxEmberEmitterCount_, 6, 1.0f, 0, 16);

    binder_->Bind("FlameThreshold", &flameThreshold_, 0.05f, 0.01f, 0.0f, 1.0f);
    binder_->Bind("FlameRatePerSquareMeter", &flameRatePerSquareMeter_, 0.4f, 0.01f, 0.0f, 20.0f);
    binder_->Bind("FlameSpawnHeight", &flameSpawnHeight_, 0.3f, 0.05f, -2.0f, 10.0f);
    binder_->Bind("MaxFlameCount", &maxFlameCount_, 1200, 10.0f, 0, 3000);

    binder_->Bind("SmokeRatePerSquareMeter", &smokeRatePerSquareMeter_, 0.04f, 0.005f, 0.0f, 5.0f);
    binder_->Bind("SmokeDuration", &smokeDuration_, 12.0f, 0.5f, 0.0f, 120.0f);
    binder_->Bind("SmokeSpawnHeight", &smokeSpawnHeight_, 1.0f, 0.1f, 0.0f, 20.0f);
    binder_->Bind("MaxSmokeCount", &maxSmokeCount_, 400, 10.0f, 0, 3000);

    binder_->Bind("MaxFireSoundCount", &maxFireSoundCount_, 3, 1.0f, 0, 8);
    binder_->Bind("FireSoundVolume", &fireSoundVolume_, 0.8f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("FireSoundMaxDistance", &fireSoundMaxDistance_, 50.0f, 1.0f, 1.0f, 500.0f);
    binder_->Bind("FireSoundFadeTime", &fireSoundFadeTime_, 1.0f, 0.05f, 0.0f, 10.0f);
    binder_->Bind("FireSoundHeight", &fireSoundHeight_, 1.0f, 0.05f, 0.0f, 10.0f);
}

void GrassFireEffects::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;

    RendererManager* rendererManager = engine_->GetRendererManager();
    const std::vector<float>& readback = rendererManager->GetGrassFireReadback();
    const uint32_t resolution = rendererManager->GetGrassFireReadbackResolution();
    FindFirePeaks(readback, resolution);

    SubmitFireLights();
    UpdateEmbers();
    UpdateFlamesAndSmoke(readback, resolution, deltaTime);
    UpdateFireSounds(deltaTime);
}

void GrassFireEffects::UpdateFireSounds(float deltaTime)
{
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    const size_t soundCount = static_cast<size_t>(std::max(maxFireSoundCount_, 0));
    if (fireSounds_.size() < soundCount)
    {
        fireSounds_.resize(soundCount);
    }

    // 鳴っている音は、近くの塊についていく
    std::vector<bool> isPeakUsed(peaks_.size(), false);
    const float followDistanceSq = (peakSeparation_ * 1.5f) * (peakSeparation_ * 1.5f);
    const Vector3 heightOffset = { 0.0f, fireSoundHeight_, 0.0f };
    for (size_t soundIndex = 0; soundIndex < fireSounds_.size(); ++soundIndex)
    {
        FireSound& sound = fireSounds_[soundIndex];
        sound.targetVolume = 0.0f;
        if (sound.handle < 0 || soundIndex >= soundCount)
        {
            continue;
        }

        int nearestPeak = -1;
        float nearestDistanceSq = followDistanceSq;
        for (size_t peakIndex = 0; peakIndex < peaks_.size(); ++peakIndex)
        {
            if (isPeakUsed[peakIndex]) continue;
            const float offsetX = peaks_[peakIndex].position.x - sound.targetPosition.x;
            const float offsetZ = peaks_[peakIndex].position.z - sound.targetPosition.z;
            const float distanceSq = offsetX * offsetX + offsetZ * offsetZ;
            if (distanceSq < nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                nearestPeak = static_cast<int>(peakIndex);
            }
        }
        if (nearestPeak >= 0)
        {
            isPeakUsed[nearestPeak] = true;
            sound.targetPosition = peaks_[nearestPeak].position + heightOffset;
            sound.targetVolume = fireSoundVolume_ * peaks_[nearestPeak].intensity;
        }
    }

    // 音のついていない塊に、空いている枠から新しく鳴らす
    for (size_t peakIndex = 0; peakIndex < peaks_.size(); ++peakIndex)
    {
        if (isPeakUsed[peakIndex]) continue;

        const auto freeSound = std::find_if(fireSounds_.begin(), fireSounds_.begin() + soundCount,
            [](const FireSound& sound) { return sound.handle < 0; });
        if (freeSound == fireSounds_.begin() + soundCount)
        {
            break;
        }

        // 小さな音から始めて、ゆっくり大きくする
        const Vector3 position = peaks_[peakIndex].position + heightOffset;
        freeSound->handle = audioPlayer.Play3D("fireLoop", position, { .volume = 0.0f, .maxDistance = fireSoundMaxDistance_, .isLoop = true });
        freeSound->position = position;
        freeSound->targetPosition = position;
        freeSound->volume = 0.0f;
        freeSound->targetVolume = fireSoundVolume_ * peaks_[peakIndex].intensity;
        isPeakUsed[peakIndex] = true;
    }

    // 大きさと場所を滑らかに変え、消えきった音は止める
    const float volumeStep = (fireSoundFadeTime_ > 0.0f) ? deltaTime / fireSoundFadeTime_ * std::max(fireSoundVolume_, 0.01f) : 10.0f;
    const float followRatio = std::min(deltaTime * 2.0f, 1.0f);
    for (FireSound& sound : fireSounds_)
    {
        if (sound.handle < 0) continue;

        sound.volume = (sound.volume < sound.targetVolume)
            ? std::min(sound.volume + volumeStep, sound.targetVolume)
            : std::max(sound.volume - volumeStep, sound.targetVolume);
        if (sound.volume <= 0.0f && sound.targetVolume <= 0.0f)
        {
            audioPlayer.Stop(sound.handle);
            sound.handle = -1;
            continue;
        }

        sound.position = sound.position + (sound.targetPosition - sound.position) * followRatio;
        audioPlayer.SetEmitterPosition(sound.handle, sound.position);
        audioPlayer.SetVolume(sound.handle, sound.volume);
    }
}

void GrassFireEffects::StopFireSounds()
{
    for (FireSound& sound : fireSounds_)
    {
        if (sound.handle >= 0)
        {
            AudioPlayer::GetInstance().Stop(sound.handle);
            sound.handle = -1;
        }
    }
}

void GrassFireEffects::FindFirePeaks(const std::vector<float>& readback, uint32_t resolution)
{
    peaks_.clear();
    if (readback.empty() || resolution == 0 || readback.size() < static_cast<size_t>(resolution) * resolution)
    {
        return;
    }

    // 強い順に並べ、近すぎる塊は 1 つにまとめる
    std::vector<std::pair<float, uint32_t>> candidates;
    for (uint32_t index = 0; index < resolution * resolution; ++index)
    {
        if (readback[index] > peakThreshold_)
        {
            candidates.emplace_back(readback[index], index);
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    const GrassFireConstants& fire = engine_->GetRendererManager()->GetGrassFireSettings();
    const size_t maxPeakCount = static_cast<size_t>(std::max(maxLightCount_, maxEmberEmitterCount_));
    const float separationSq = peakSeparation_ * peakSeparation_;

    for (const auto& [intensity, index] : candidates)
    {
        if (peaks_.size() >= maxPeakCount) break;

        const float u = (static_cast<float>(index % resolution) + 0.5f) / static_cast<float>(resolution);
        const float v = (static_cast<float>(index / resolution) + 0.5f) / static_cast<float>(resolution);
        Vector3 position = {
            fire.areaCenter.x + (u - 0.5f) * fire.areaSize.x,
            0.0f,
            fire.areaCenter.y + (v - 0.5f) * fire.areaSize.y };

        const bool isTooClose = std::any_of(peaks_.begin(), peaks_.end(), [&](const FirePeak& peak)
            {
                const float offsetX = peak.position.x - position.x;
                const float offsetZ = peak.position.z - position.z;
                return offsetX * offsetX + offsetZ * offsetZ < separationSq;
            });
        if (isTooClose) continue;

        position.y = terrain_ ? terrain_->GetHeight(position.x, position.z) : 0.0f;
        peaks_.push_back({ position, std::min(intensity, 1.0f) });
    }
}

void GrassFireEffects::SubmitFireLights()
{
    LightManager* lightManager = engine_->GetLightManager();
    const size_t lightCount = std::min(peaks_.size(), static_cast<size_t>(std::max(maxLightCount_, 0)));

    for (size_t index = 0; index < lightCount; ++index)
    {
        const FirePeak& peak = peaks_[index];

        // 速さの違う揺れを重ね、炎らしく不規則にちらつかせる
        const float phase = static_cast<float>(index) * 1.7f;
        const float flicker = 0.75f + 0.15f * std::sin(elapsedTime_ * 11.0f + phase) + 0.1f * std::sin(elapsedTime_ * 23.0f + phase * 2.3f);

        lightManager->SubmitPointLight(
            peak.position + Vector3(0.0f, lightHeight_, 0.0f),
            lightColor_,
            lightIntensity_ * peak.intensity * flicker,
            lightRadius_,
            lightVolumetricIntensity_);
    }
}

void GrassFireEffects::UpdateEmbers()
{
    const size_t emitterCount = static_cast<size_t>(std::max(maxEmberEmitterCount_, 0));

    // 足りない分だけ作る。作ったものは止めて使い回す
    while (emberEmitters_.size() < emitterCount)
    {
        ParticleEmitter* emitter = engine_->GetParticleSystem()->SpawnEmitter("grassFireEmbers");
        if (!emitter) break;
        emberEmitters_.push_back(emitter);
        isEmberPlaying_.push_back(false);
    }

    for (size_t index = 0; index < emberEmitters_.size(); ++index)
    {
        ParticleEmitter* emitter = emberEmitters_[index];
        const bool shouldPlay = index < emitterCount && index < peaks_.size();
        if (!shouldPlay)
        {
            if (isEmberPlaying_[index])
            {
                emitter->Stop();
                isEmberPlaying_[index] = false;
            }
            continue;
        }

        emitter->SetPosition(peaks_[index].position);
        emitter->SetEmissionRateMultiplier(peaks_[index].intensity);
        if (!isEmberPlaying_[index])
        {
            emitter->Play();
            isEmberPlaying_[index] = true;
        }
    }
}

Vector3 GrassFireEffects::CalculateRandomPositionInCell(uint32_t cellIndex, uint32_t resolution) const
{
    const GrassFireConstants& fire = engine_->GetRendererManager()->GetGrassFireSettings();
    const float u = (static_cast<float>(cellIndex % resolution) + Math::RandomFloat(0.0f, 1.0f)) / static_cast<float>(resolution);
    const float v = (static_cast<float>(cellIndex / resolution) + Math::RandomFloat(0.0f, 1.0f)) / static_cast<float>(resolution);
    Vector3 position = {
        fire.areaCenter.x + (u - 0.5f) * fire.areaSize.x,
        0.0f,
        fire.areaCenter.y + (v - 0.5f) * fire.areaSize.y };
    position.y = terrain_ ? terrain_->GetHeight(position.x, position.z) : 0.0f;
    return position;
}

void GrassFireEffects::UpdateFlamesAndSmoke(const std::vector<float>& readback, uint32_t resolution, float deltaTime)
{
    const size_t cellCount = static_cast<size_t>(resolution) * resolution;

    // 燃え跡を消したときは地図が空になるので、覚えていた燃え方も消す
    if (readback.empty() || resolution == 0 || readback.size() < cellCount)
    {
        cellLastBurningTime_.clear();
        flameCredits_.clear();
        smokeCredits_.clear();
        estimatedFlameCount_ = 0;
        estimatedSmokeCount_ = 0;
        return;
    }
    if (cellLastBurningTime_.size() != cellCount)
    {
        cellLastBurningTime_.assign(cellCount, -1.0f);
        flameCredits_.assign(cellCount, 0.0f);
        smokeCredits_.assign(cellCount, 0.0f);
    }

    // 1 マスの広さ
    const GrassFireConstants& fire = engine_->GetRendererManager()->GetGrassFireSettings();
    const float cellArea = (fire.areaSize.x / static_cast<float>(resolution)) * (fire.areaSize.y / static_cast<float>(resolution));

    // 燃えているマスと煙を出すマスだけを集め、出す速さを決める
    activeCells_.clear();
    float totalFlameRate = 0.0f;
    float totalSmokeRate = 0.0f;
    for (size_t index = 0; index < cellCount; ++index)
    {
        const float fireStrength = std::min(readback[index], 1.0f);
        const bool isBurning = fireStrength > flameThreshold_;
        if (isBurning)
        {
            cellLastBurningTime_[index] = elapsedTime_;
        }
        if (cellLastBurningTime_[index] < 0.0f)
        {
            continue;
        }

        // 燃えている間は一番多く、燃え尽きてからはだんだん減らす
        const float timeSinceBurning = elapsedTime_ - cellLastBurningTime_[index];
        const float smokeRatio = isBurning ? 1.0f : ((smokeDuration_ > 0.0f) ? std::clamp(1.0f - timeSinceBurning / smokeDuration_, 0.0f, 1.0f) : 0.0f);
        const float flameRate = isBurning ? flameRatePerSquareMeter_ * cellArea * fireStrength : 0.0f;
        const float smokeRate = smokeRatePerSquareMeter_ * cellArea * smokeRatio;
        if (flameRate <= 0.0f && smokeRate <= 0.0f)
        {
            continue;
        }

        activeCells_.push_back({ static_cast<uint32_t>(index), flameRate, smokeRate });
        totalFlameRate += flameRate;
        totalSmokeRate += smokeRate;
    }

    // 寿命はパーティクルエディタの生存時間。実行中に変えるとすぐ反映
    ParticleSystem* particleSystem = engine_->GetParticleSystem();
    auto GetPresetLifetime = [particleSystem](const char* presetName)
        {
            const auto found = particleSystem->definitions_.find(presetName);
            return (found != particleSystem->definitions_.end()) ? found->second.emitterConfig.lifetime : 0.0f;
        };
    const float flameLifetime = GetPresetLifetime("grassFireFlame");
    const float smokeLifetime = GetPresetLifetime("grassFireSmoke");

    // 出ている数
    const float flameCountWithoutLimit = totalFlameRate * flameLifetime;
    const float smokeCountWithoutLimit = totalSmokeRate * smokeLifetime;
    const float flameScale = (flameCountWithoutLimit > static_cast<float>(maxFlameCount_)) ? static_cast<float>(maxFlameCount_) / flameCountWithoutLimit : 1.0f;
    const float smokeScale = (smokeCountWithoutLimit > static_cast<float>(maxSmokeCount_)) ? static_cast<float>(maxSmokeCount_) / smokeCountWithoutLimit : 1.0f;
    estimatedFlameCount_ = static_cast<int>(flameCountWithoutLimit * flameScale);
    estimatedSmokeCount_ = static_cast<int>(smokeCountWithoutLimit * smokeScale);

    WorldTransform spawnTransform;
    for (const ActiveCell& cell : activeCells_)
    {
        float& flameCredit = flameCredits_[cell.index];
        flameCredit += cell.flameRate * flameScale * deltaTime;
        while (flameCredit >= 1.0f)
        {
            flameCredit -= 1.0f;
            spawnTransform.translation_ = CalculateRandomPositionInCell(cell.index, resolution) + Vector3(0.0f, flameSpawnHeight_, 0.0f);
            particleSystem->SpawnParticle(spawnTransform, "grassFireFlame", flameLifetime, nullptr);
        }

        float& smokeCredit = smokeCredits_[cell.index];
        smokeCredit += cell.smokeRate * smokeScale * deltaTime;
        while (smokeCredit >= 1.0f)
        {
            smokeCredit -= 1.0f;
            spawnTransform.translation_ = CalculateRandomPositionInCell(cell.index, resolution) + Vector3(0.0f, smokeSpawnHeight_, 0.0f);
            particleSystem->SpawnParticle(spawnTransform, "grassFireSmoke", smokeLifetime, nullptr);
        }
    }
}

void GrassFireEffects::DebugGui()
{
    ImGui::Begin("延焼の演出");

    ImGui::Text("燃えている塊: %d / 炎: 約 %d / 煙: 約 %d", static_cast<int>(peaks_.size()), estimatedFlameCount_, estimatedSmokeCount_);

    ImGui::SeparatorText("燃えている塊の見つけ方");
    binder_->Draw("PeakThreshold", "これより弱い炎は数えない");
    binder_->Draw("PeakSeparation", "塊どうしの最短の間隔 (m)");

    ImGui::SeparatorText("炎の光");
    binder_->Draw("MaxLightCount", "ライトの数");
    binder_->Draw("LightColor", "色");
    binder_->Draw("LightIntensity", "強さ");
    binder_->Draw("LightRadius", "届く距離");
    binder_->Draw("LightHeight", "地面からの高さ");
    binder_->Draw("LightVolumetricIntensity", "霧の中の光の筋の強さ");

    ImGui::SeparatorText("火の粉");
    binder_->Draw("MaxEmberEmitterCount", "出す場所の数");

    ImGui::SeparatorText("炎 (grassFireFlame)");
    binder_->Draw("FlameThreshold", "これより弱いマスには出さない");
    binder_->Draw("FlameRatePerSquareMeter", "1 m² で 1 秒に出す数 (炎が一番強いとき)");
    binder_->Draw("FlameSpawnHeight", "出す高さ (m)");
    binder_->Draw("MaxFlameCount", "同時に出す数の上限");

    ImGui::SeparatorText("煙 (grassFireSmoke)");
    binder_->Draw("SmokeRatePerSquareMeter", "1 m² で 1 秒に出す数 (燃えている間)");
    binder_->Draw("SmokeDuration", "燃え尽きてから出し続ける秒数");
    binder_->Draw("SmokeSpawnHeight", "出す高さ (m)");
    binder_->Draw("MaxSmokeCount", "同時に出す数の上限");

    ImGui::SeparatorText("燃える音");
    binder_->Draw("MaxFireSoundCount", "同時に鳴らす数");
    binder_->Draw("FireSoundVolume", "大きさ");
    binder_->Draw("FireSoundMaxDistance", "聞こえる距離 (m)");
    binder_->Draw("FireSoundFadeTime", "鳴り始めと消えるときの秒数");
    binder_->Draw("FireSoundHeight", "地面からの高さ (m)");

    ImGui::End();
}
