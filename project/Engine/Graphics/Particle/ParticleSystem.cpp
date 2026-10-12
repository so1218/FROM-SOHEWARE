#include "pch.h"
#include "ParticleSystem.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "ParticleEditor.h"
#include "ParticleConfigManager.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "EnvironmentManager.h"

#include "externals/FastNoiseLite.h"

namespace FE
{

namespace
{
    FastNoiseLite gNoiseGen;
}

ParticleSystem::ParticleSystem(Engine* engine)
{
    engine_ = engine;

    // エディタと設定マネージャを生成
    editor_ = std::make_unique<ParticleEditor>(this, engine_);
    configManager_ = std::make_unique<ParticleConfigManager>(this);
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Initialize()
{
    // 全パーティクル設定をロード
    configManager_->LoadAllParticleDefinitions();

    // ロードした全プリセットのテクスチャハンドルを一括更新
    for (auto& [name, definition] : definitions_)
    {
        definition.particleConfig.RefreshResources();
    }
}

void ParticleSystem::SpawnParticle(
    const WorldTransform& transform,
    const std::string& presetName,
    float lifetime,
    const WorldTransform* attractionTarget,
    const WorldTransform* vortexTarget,
    const ModelData* emitterModelData,
    const AnimationModel* emitterAnimModel)
{
    if (particles_.size() >= engine_->GetRendererManager()->GetMaxParticleCount()) return;

    // パーティクル設定を取得
    const auto& config = GetConfig(presetName);

    // 不可視チェック
    uint32_t startA = (config.colorOverLifetime.startColor) & 0xFF;
    uint32_t endA = (config.colorOverLifetime.endColor) & 0xFF;
    bool invisibleAlpha = (startA < 5 && endA < 5);

    bool startZero = (config.sizeOverLifetime.startScale.x <= 0.0f || config.sizeOverLifetime.startScale.y <= 0.0f);
    bool endZero = (config.sizeOverLifetime.endScale.x <= 0.0f || config.sizeOverLifetime.endScale.y <= 0.0f);
    bool invisibleScale = (startZero && endZero);

    if (!config.trail.enabled && (invisibleAlpha || invisibleScale)) return;

    ParticleState particle;
    particle.config = &config; 

    // Shape オフセット計算
    Vector3 localOffset = config.shape.GetInitialPositionOffset(emitterModelData, emitterAnimModel);
    Matrix4x4 transformMatrix = Matrix4x4::MakeAffine(transform.scale_, transform.rotationQuaternion_, { 0.0f, 0.0f, 0.0f });
    Vector3 worldOffset = transformMatrix.TransformVector(localOffset);

    particle.transform.translation_ = transform.translation_ + worldOffset;
    particle.initialPosition = particle.transform.translation_;

    // Velocity
    if (config.velocity.enabled)
    {
        Vector3 localVel = config.velocity.GetInitialVelocity();
        particle.velocity = transform.rotationQuaternion_.RotateVector(localVel);
    }

    // Color & Size
    particle.color = config.colorOverLifetime.enabled ? config.colorOverLifetime.Evaluate(0.0f) : config.baseColor;
    particle.transform.scale_ = config.sizeOverLifetime.enabled ? config.sizeOverLifetime.Evaluate(0.0f) : Vector3{ 1.0f, 1.0f, 1.0f };

    // Rotation
    if (config.rotation.enabled)
    {
        const auto& rotConfig = config.rotation;
        Vector3 startRot = {
            Math::RandomFloat(rotConfig.minStartRotation.x, rotConfig.maxStartRotation.x),
            Math::RandomFloat(rotConfig.minStartRotation.y, rotConfig.maxStartRotation.y),
            Math::RandomFloat(rotConfig.minStartRotation.z, rotConfig.maxStartRotation.z)
        };

        particle.transform.rotation_ = {
            Math::ToRadians(startRot.x),
            Math::ToRadians(startRot.y),
            Math::ToRadians(startRot.z)
        };

        if (rotConfig.isBillboard)
        {
            float velZ = Math::RandomFloat(rotConfig.minAngularVelocity2D, rotConfig.maxAngularVelocity2D);
            particle.currentAngularVelocity = { 0.0f, 0.0f, velZ };
        }
        else
        {
            particle.currentAngularVelocity = {
                Math::RandomFloat(rotConfig.minAngularVelocity3D.x, rotConfig.maxAngularVelocity3D.x),
                Math::RandomFloat(rotConfig.minAngularVelocity3D.y, rotConfig.maxAngularVelocity3D.y),
                Math::RandomFloat(rotConfig.minAngularVelocity3D.z, rotConfig.maxAngularVelocity3D.z)
            };
        }
    }

    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.attractionTarget = attractionTarget;
    particle.vortexTarget = vortexTarget;
    particle.trailSeed = Math::RandomFloat(0.0f, 1000.0f);

    particles_.push_back(std::move(particle));
}

std::unique_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(const std::string& presetName)
{
    // 定義が存在しなければデフォルトを作成して保存
    if (definitions_.find(presetName) == definitions_.end())
    {
        definitions_[presetName] = ParticleDefinition();
        configManager_->SaveParticleDefinitionToJson(presetName);
    }

    const auto& definition = definitions_.at(presetName);
    const auto& emitterConfig = definition.emitterConfig;

    auto emitter = std::make_unique<ParticleEmitter>();
    emitter->presetName_ = presetName; // プリセット名を保持
    emitter->Initialize(emitterConfig); // 設定で初期化

    return emitter;
}

void ParticleSystem::Update()
{
    // ---------------------------------------------------------
    // エミッター更新
    // ---------------------------------------------------------
    auto it = emitters_.begin();
    while (it != emitters_.end())
    {
        auto& emitter = *it;
        if (emitter->isDead_)
        {
            std::string name = emitter->presetName_;
            it = emitters_.erase(it);
            namedEmitters_.erase(name);
            continue;
        }
        emitter->Update(*this);
        ++it;
    }

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    float totalTime = TimeManager::GetInstance()->GetTotalTime();

    // ---------------------------------------------------------
    // 風・ノイズパラメータの事前計算
    // ---------------------------------------------------------
    auto envManager = EnvironmentManager::GetInstance();
    Vector2 globalWindDir = envManager->GetWindDirection();
    float globalWindSpeed = envManager->GetWindSpeed();
    Vector3 baseWindVelocity = { globalWindDir.x * globalWindSpeed, 0.0f, globalWindDir.y * globalWindSpeed };

    gNoiseGen.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

    // ---------------------------------------------------------
    // パーティクル更新 ＆ GPU送信
    // ---------------------------------------------------------
    for (size_t i = 0; i < particles_.size(); )
    {
        ParticleState& particleState = particles_[i];

        // 安全ガード：設定ポインタが無効なら削除
        if (!particleState.config)
        {
            particles_[i] = std::move(particles_.back());
            particles_.pop_back();
            continue;
        }

        const ParticleConfig& config = *particleState.config;

        particleState.age += deltaTime;

        bool isExpired = (particleState.age >= particleState.lifetime);
        bool isTrailEmpty = particleState.trailHistory.empty();
        bool shouldDelete = config.trail.enabled ? (isExpired && isTrailEmpty) : isExpired;

        // Swap & Pop-back による O(1) 削除
        if (shouldDelete)
        {
            particles_[i] = std::move(particles_.back());
            particles_.pop_back();
            continue; // i をインクリメントせず次のループへ
        }

        if (!isExpired)
        {
            float t = particleState.lifetime > 0.0f ? (particleState.age / particleState.lifetime) : 1.0f;

            // Noise Module
            if (config.noise.enabled)
            {
                const auto& noiseConfig = config.noise;
                gNoiseGen.SetFrequency(noiseConfig.frequency);

                float scroll = totalTime * noiseConfig.scrollSpeed;
                Vector3 samplePos = particleState.transform.translation_;
                samplePos.y -= scroll;

                Vector3 noiseForce = { 0.0f, 0.0f, 0.0f };

                if (noiseConfig.type == NoiseModule::NoiseType::Curl)
                {
                    const float e = 0.01f;
                    float n1 = gNoiseGen.GetNoise(samplePos.x, samplePos.y + e, samplePos.z);
                    float n2 = gNoiseGen.GetNoise(samplePos.x, samplePos.y - e, samplePos.z);
                    float n3 = gNoiseGen.GetNoise(samplePos.x, samplePos.y, samplePos.z + e);
                    float n4 = gNoiseGen.GetNoise(samplePos.x, samplePos.y, samplePos.z - e);
                    float n5 = gNoiseGen.GetNoise(samplePos.x + e, samplePos.y, samplePos.z);
                    float n6 = gNoiseGen.GetNoise(samplePos.x - e, samplePos.y, samplePos.z);

                    noiseForce.x = (n1 - n2) - (n3 - n4);
                    noiseForce.y = (n3 - n4) - (n5 - n6);
                    noiseForce.z = (n5 - n6) - (n1 - n2);
                }
                else
                {
                    if (noiseConfig.separateAxes)
                    {
                        noiseForce.x = gNoiseGen.GetNoise(samplePos.x, samplePos.y, samplePos.z);
                        noiseForce.y = gNoiseGen.GetNoise(samplePos.x + 100.0f, samplePos.y + 100.0f, samplePos.z + 100.0f);
                        noiseForce.z = gNoiseGen.GetNoise(samplePos.x + 200.0f, samplePos.y + 200.0f, samplePos.z + 200.0f);
                    }
                    else
                    {
                        float val = gNoiseGen.GetNoise(samplePos.x, samplePos.y, samplePos.z);
                        noiseForce = { val, val, val };
                    }
                }

                particleState.velocity += noiseForce * noiseConfig.strength * deltaTime;
            }

            // Physics Module
            if (config.physics.enabled)
            {
                particleState.velocity += config.physics.gravity * deltaTime;

                if (config.physics.affectedByWind && globalWindSpeed > 0.0f)
                {
                    particleState.velocity += baseWindVelocity * config.physics.windInfluence * deltaTime;
                }

                particleState.velocity *= (1.0f - config.physics.drag * deltaTime);
            }

            // Vortex Module
            if (config.vortex.enabled)
            {
                Vector3 vortexCenter = particleState.vortexTarget ?
                    (particleState.vortexTarget->translation_ + config.vortex.offset) :
                    (particleState.initialPosition + config.vortex.center);

                Vector3 diff = particleState.transform.translation_ - vortexCenter;
                Vector3 axis = config.vortex.axis.Normalize();

                float height = diff.Dot(axis);
                Vector3 radialVector = diff - (axis * height);
                float distanceToAxis = radialVector.Length();

                if (distanceToAxis > 0.01f)
                {
                    Vector3 radialDir = radialVector.Normalize();
                    Vector3 tangentialDir = axis.Cross(radialDir).Normalize();

                    Vector3 axialVelocity = axis * particleState.velocity.Dot(axis);
                    Vector3 orbitalVelocity = tangentialDir * config.vortex.orbitalSpeed;
                    Vector3 radialVelocity = radialDir * config.vortex.radialSpeed;

                    Vector3 targetVelocity = axialVelocity + orbitalVelocity + radialVelocity;
                    float lerpRate = std::min(10.0f * deltaTime, 1.0f);

                    particleState.velocity = Math::Lerp(particleState.velocity, targetVelocity, lerpRate);
                }
            }

            // Attraction Module
            if (config.attraction.enabled)
            {
                Vector3 targetPos = particleState.attractionTarget ?
                    (particleState.attractionTarget->translation_ + config.attraction.offset) :
                    config.attraction.target;

                Vector3 directionToTarget = targetPos - particleState.transform.translation_;
                particleState.velocity += directionToTarget.Normalize() * config.attraction.strength * deltaTime;
            }

            // 位置更新
            particleState.transform.translation_ += particleState.velocity * deltaTime;

            // Rotation Module
            if (config.rotation.enabled)
            {
                Vector3 angularVelocity = particleState.currentAngularVelocity;

                if (config.rotation.isBillboard)
                {
                    particleState.transform.rotation_.x = 0.0f;
                    particleState.transform.rotation_.y = 0.0f;
                    particleState.transform.rotation_.z += Math::ToRadians(angularVelocity.z) * deltaTime;
                }
                else
                {
                    Vector3 velocityRadians = {
                        Math::ToRadians(angularVelocity.x),
                        Math::ToRadians(angularVelocity.y),
                        Math::ToRadians(angularVelocity.z)
                    };
                    particleState.transform.rotation_ += velocityRadians * deltaTime;

                    // クォータニオン変換
                    particleState.transform.rotationQuaternion_ = Quaternion::QuaternionFromEuler(particleState.transform.rotation_);
                }
            }

            // Color & Size Module
            if (config.colorOverLifetime.enabled)
                particleState.color = config.colorOverLifetime.Evaluate(t);

            if (config.sizeOverLifetime.enabled)
                particleState.transform.scale_ = config.sizeOverLifetime.Evaluate(t);
        }

        // Trail Module
        if (config.trail.enabled)
        {
            Vector3 currentPos = particleState.transform.translation_;

            if (!isExpired)
            {
                bool shouldAdd = particleState.trailHistory.empty();
                if (!shouldAdd)
                {
                    Vector3 lastPos = particleState.trailHistory.back().position;
                    float distSq = (currentPos - lastPos).LengthSq();
                    if (distSq >= config.trail.minVertexDistance * config.trail.minVertexDistance)
                        shouldAdd = true;
                }

                if (shouldAdd)
                {
                    TrailPoint newPoint;
                    newPoint.position = currentPos;
                    newPoint.rotationQuaternion = particleState.transform.rotationQuaternion_;
                    newPoint.time = particleState.age;
                    particleState.trailHistory.push_back(newPoint);
                }
            }

            while (!particleState.trailHistory.empty())
            {
                float timeAlive = particleState.age - particleState.trailHistory.front().time;
                if (timeAlive > config.trail.lifetime)
                    particleState.trailHistory.pop_front();
                else
                    break;
            }
        }

        // GPUサブミット
        if (!isExpired && !ShouldSkipDraw(particleState))
        {
            particleState.transform.UpdateMatrix();

            engine_->GetRendererManager()->SubmitParticleInstance(
                particleState.transform,
                Math::ColorVectorToUint32(particleState.color),
                config.textureSheet.textureHandle,
                particleState.transform.rotation_.z,
                config.blendMode,
                config.rotation.isBillboard,
                config.intensity
            );
        }

        ++i;
    }
}

void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter> emitter)
{
    const std::string& name = emitter->presetName_;

    namedEmitters_[name] = emitter.get();

    emitters_.push_back(std::move(emitter));
}

void ParticleSystem::Draw()
{
    engine_->SetBlendMode(BlendMode::kBlendModeAdd);

    // トレイル描画
    for (const auto& particle : particles_)
    {
        // ポインタチェック
        if (!particle.config || !particle.config->trail.enabled) continue; // トレイル無効ならスキップ
        if (particle.trailHistory.size() < 2) continue;                    // 頂点が足りなければスキップ

        // 描画用ポイントリストを作成
        std::vector<TrailPoint> drawPoints;
        drawPoints.reserve(particle.trailHistory.size() + 1);

        for (const auto& tp : particle.trailHistory)
            drawPoints.push_back(tp);

        // 生存中なら現在位置も追加
        if (particle.age < particle.lifetime)
            drawPoints.push_back({ particle.transform.translation_, particle.transform.rotationQuaternion_, particle.age });

        // Rendererに登録
        engine_->GetRendererManager()->SubmitTrail(drawPoints, particle.config->trail, particle.trailSeed);
    }

    // ブレンドを元に戻す
    engine_->SetBlendMode(BlendMode::kBlendModeNormal);

#ifdef ENABLE_IMGUI
    // デバッグ用エディタ表示
    editor_->ShowEditor();
#endif
}

void ParticleSystem::Clear()
{
    // すべてのアクティブなパーティクルを削除
    particles_.clear();

    // すべてのエミッターを削除
    emitters_.clear();

    // 名前付きエミッターのマップをクリア
    namedEmitters_.clear();
}

std::vector<std::string> ParticleSystem::GetPresetNames() const
{
    std::vector<std::string> names;
    names.reserve(definitions_.size());
    for (const auto& [name, def] : definitions_)
    {
        names.push_back(name);
    }
    return names;
}

bool ParticleSystem::ShouldSkipDraw(const ParticleState& particle) const
{
    // ポインタチェック
    if (!particle.config) return true;

    const auto& config = *particle.config;

    // アルファ値のチェック
    uint32_t startA = (config.colorOverLifetime.startColor) & 0xFF;
    uint32_t endA = (config.colorOverLifetime.endColor) & 0xFF;
    if (startA < 5 && endA < 5) return true;

    // スケールのチェック
    bool startZero = (config.sizeOverLifetime.startScale.x == 0.0f || config.sizeOverLifetime.startScale.y == 0.0f);
    bool endZero = (config.sizeOverLifetime.endScale.x == 0.0f || config.sizeOverLifetime.endScale.y == 0.0f);
    if (startZero && endZero) return true;

    return false;
}

}