#define _USE_MATH_DEFINES

#include "Particle.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "TextureHandle.h"
#include "json.hpp"

// ParticleType文字列をenumに変換する関数
ParticleType StringToParticleType(const std::string& typeStr)
{
    // 他のタイプも追加
    return ParticleType::None;
}

void ParticleSystem::Initialize(Engine* engine)
{
    engine_ = engine;

    // JSONから読み込む
    LoadParticleDefinitionsFromJson("Game/Data/particles.json");

    // JSONに色情報がない場合、ここでデフォルト色を設定する
    particleConfigs_[static_cast<size_t>(ParticleType::None)].baseColor = Uint32ToColorVector(0xFFFFFFff);
}

void ParticleSystem::SpawnParticle(WorldTransform& transform, ParticleType type, float lifetime, int amount)
{
    if (particles_.size() >= engine_->kMaxParticleCount) return;

    const ParticleConfig* config = &particleConfigs_[static_cast<size_t>(type)];

    ParticleState particle;
    particle.transform = std::make_unique<WorldTransform>(transform);
    particle.color = config->baseColor;
    particle.type = type;
    particle.amount = amount;
    particle.textureHandle = config->textureIndex;
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.hasLifetime = true;

    // タイプごとの初期値
    if (type == ParticleType::Key)
    {
        particle.appearInterval = 4;
        particle.emitterRange = { 0.5f, 1,0 };
        particle.fadeOutEase->interval_ = 0.01f;
        particle.color = { 127.0f,255.0f,0.0f,255.0f };
        particle.startColor = 0xffa500ff;
        particle.endColor = 0xffa50000;
        particle.textureHandle = TextureID::white1x1;
        particle.isExist = true;
        particle.hasExisted = false;
        particle.frameCount = 0;
        particle.isEmit = false;
        particle.speed = 0.01f;
        particle.fadeOutEase->SetEasing(EasingType::EaseOutCirc);
        particle.scaleEase->SetEasing(EasingType::EaseLinear);
        particle.fadeOutEase->SetEaseDurationFrames(250);
        particle.scaleEase->interval_=0.04f;
    }

    particles_.push_back(std::move(particle));
}

void ParticleSystem::LoadParticleDefinitionsFromJson(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        // エラー処理: ファイルが開けなかったことをログに出力するなど
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return;
    }

    for (const auto& particle_json : j) {
        ParticleConfig config = {};
        config.type = StringToParticleType(particle_json["type"].get<std::string>());
        config.gravity = particle_json["gravity"].get<float>();
        config.drag = particle_json["drag"].get<float>();
        config.decayRate = particle_json["decayRate"].get<float>();
        config.maxLifetime = particle_json["maxLifetime"].get<float>();
        config.textureIndex = particle_json["textureIndex"].get<uint32_t>();

        particleConfigs_[static_cast<size_t>(config.type)] = config;

    }
}

void ParticleSystem::Update()
{
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    for (auto& emitter : emitters_)
    {
        emitter->Update(deltaTime, *this);
    }

    // 既存のパーティクル更新処理
    for (auto it = particles_.begin(); it != particles_.end(); )
    {
        ParticleType type = it->type;

        if (type == ParticleType::Key)
        {


            if (it->frameCount >= it->amount * it->appearInterval)
            {
                it->hasExisted = false;
                it->frameCount = 0;
            }
            // フレームごとに新しいパーティクルを生成
            if (!it->hasExisted)
            {
                if (it->frameCount >= it->spawnFrame_)
                {
                    if (!it->isExist)
                    {
                        // ランダムで角度を設定
                        it->theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * float(M_PI);

                        // 半径をランダムに生成 (0～emitterRange_ の範囲)
                        float radius = static_cast<float>(RandomFloat(0.05f, static_cast<float>(it->emitterRange.x)));

                        // 極座標 -> 直交座標
                        it->transform->translation_.x = static_cast<float>(it->transform->translation_.x) + radius * cos(it->theta);
                        it->transform->translation_.y = static_cast<float>(it->transform->translation_.y + 0.5f) + radius * sin(it->theta);
                        it->velocity.x = it->speed * cosf(it->theta);
                        it->velocity.y = it->speed * sinf(it->theta);
                        it->isExist = true;
                        it->fadeOutEase->isEase_ = true;
                        it->hasExisted = true;
                        if (rand() % 1 == 0)
                        {
                            it->color = { 225.0f,185.0f,0.0f,255.0f };
                            it->startColor = 0xFFB900ff;
                            it->endColor = 0xFFB90000;
                        }
                        else if (rand() % 1 == 1)
                        {
                            it->color = { 12.0f,174.0f,112.0f,255.0f };
                            it->startColor = 0xCAE700ff;
                            it->endColor = 0xCAE70000;
                        }

                        if (rand() % 2 == 0)
                        {
                            it->thetaVel = float(rand() % 2 + 0.01f);
                        }
                        else
                        {
                            it->thetaVel = -float(rand() % 2 + 0.01f);
                        }

                    }
                    it->frameCount = 0;
                }
            }
            it->frameCount++;
            if (it->isExist)
            {
                it->transform->translation_.x += it->velocity.x;
                it->transform->translation_.y += it->velocity.y;

                it->transform->rotation_.z += it->thetaVel;
                it->transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(it->transform->rotation_);

                if (!it->fadeOutEase->isEase_)
                {
                    it->isExist = false;
                }
                unsigned int currentColor = (unsigned int)ColorVectorToUint32(it->color);
                if (it->fadeOutEase->isEase_)
                {
                    it->fadeOutEase->CountEaseLinear(it->startColor, it->endColor, currentColor);
                }
                it->color = Uint32ToColorVector(currentColor);
                it->scaleEase->OnceReverseEaseLinear({ 0.0f,0.0f, 0.0f }, { 2.0f,2.0f,0.1f }, it->transform->scale_);
            }
        }
        if (it->hasLifetime)
        {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0.0f)
            {
                it = particles_.erase(it); // 寿命が尽きたパーティクルを消去
                continue;
            }
        }
        ++it;
    }

    // パーティクルインスタンスの更新
    for (auto& particle : particles_)
    {
        particle.transform->UpdateMatrix();
        engine_->SubmitParticleInstance(*particle.transform, ColorVectorToUint32(particle.color), particle.textureHandle, particle.transform->rotation_.z);
    }

}

void ParticleSystem::AddEmitter(ParticleEmitter* emitter)
{
    emitters_.push_back(emitter);
}
