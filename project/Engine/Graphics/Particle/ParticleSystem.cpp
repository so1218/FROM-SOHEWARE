#define _USE_MATH_DEFINES

#include "ParticleSystem.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "ParticleEditor.h"
#include "ParticleConfigManager.h"
#include "TextureHandle.h"
#include "ImGuiManager.h"
#include "json.hpp"

ParticleSystem::ParticleSystem()
{
    // エディタと設定マネージャを生成
    editor_ = std::make_unique<ParticleEditor>(this);
    configManager_ = std::make_unique<ParticleConfigManager>(this);
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Initialize(Engine* engine)
{
    engine_ = engine;

    // 全パーティクル設定をロード
    configManager_->LoadAllParticleDefinitions();
}

void ParticleSystem::SpawnParticle(WorldTransform& transform, const std::string& presetName, float lifetime)
{
    // 最大数を超える場合は生成しない
    if (particles_.size() >= engine_->renderer_->kMaxParticleCount) return;

    // パーティクル設定を取得
    auto& config = GetConfig(presetName);

    ParticleState particle;
    particle.config = config;

    // Shape
    // エミッタ位置 + Shapeオフセットで初期座標を設定
    particle.transform = std::make_unique<WorldTransform>();
    particle.transform->translation_ = transform.translation_ + particle.config.shape.GetInitialPositionOffset();

    // Velocity 
    if (particle.config.velocity.enabled)
        particle.velocity = particle.config.velocity.GetInitialVelocity();

    // Color
    if (particle.config.colorOverLifetime.enabled &&
        particle.config.colorOverLifetime.mode == ColorOverLifetimeModule::Mode::RandomBetweenTwo)
    {
        // 50%の確率で2つ目のグラデーションを使用
        if (Math::RandomFloat(0.0f, 1.0f) > 0.5f)
        {
            particle.config.colorOverLifetime.startColor = particle.config.colorOverLifetime.startColor2;
            particle.config.colorOverLifetime.endColor = particle.config.colorOverLifetime.endColor2;
        }
    }

    // 初期色を評価
    particle.color = particle.config.colorOverLifetime.enabled ?
        particle.config.colorOverLifetime.Evaluate(0.0f) :
        particle.config.baseColor;

    // Size
    particle.transform->scale_ = particle.config.sizeOverLifetime.enabled ?
        particle.config.sizeOverLifetime.Evaluate(0.0f) :
        Vector3{ 1.0f, 1.0f, 1.0f };

    // Rotation
    if (particle.config.rotation.enabled)
    {
        if (particle.config.rotation.isBillboard)
        {
            // ビルボード回転
            particle.transform->rotation_.z = particle.config.rotation.randomStartRotation ?
                Math::RandomFloat(0.0f, 360.0f) :
                Math::ToRadians(particle.config.rotation.orientation3D.z);
        }
        else
        {
            // 通常回転
            particle.transform->rotation_ = 
            {
                Math::ToRadians(particle.config.rotation.orientation3D.x),
                Math::ToRadians(particle.config.rotation.orientation3D.y),
                Math::ToRadians(particle.config.rotation.orientation3D.z)
            };
        }
    }
    else
    {
        // 回転無効
        particle.transform->rotation_ = { 0.0f, 0.0f, 0.0f };
    }

    // 共通プロパティ
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.presetName = presetName;

    // 生成したパーティクルを格納
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
    // エミッター更新 
    auto it = emitters_.begin();
    while (it != emitters_.end())
    {
        auto& emitter = *it;

        // エミッターが寿命切れなら削除
        if (emitter->isDead_)
        {
            std::string name = emitter->presetName_;
            it = emitters_.erase(it);

            // 名前付きエミッターも削除
            namedEmitters_.erase(name);
            continue;
        }

        // エミッターを更新して新パーティクル生成
        emitter->Update(*this);
        ++it;
    }

    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // パーティクルの更新
    for (auto particle = particles_.begin(); particle != particles_.end(); )
    {
        ParticleState& particleState = *particle;
        const ParticleConfig& config = particleState.config;

        // 寿命チェック
        particleState.age += deltaTime;

        // 削除条件
        bool isExpired = (particleState.age >= particleState.lifetime);
        bool isTrailEmpty = particleState.trailHistory.empty();

        // 削除するかどうかの判定
        bool shouldDelete = false;

        if (!config.trail.enabled)
        {
            shouldDelete = isExpired;
        }
        else
        {
            shouldDelete = (isExpired && isTrailEmpty);
        }

        if (shouldDelete)
        {
            particle = particles_.erase(particle);
            continue; 
        }

        if (!isExpired)
        {
            float t = particleState.lifetime > 0.0f ? (particleState.age / particleState.lifetime) : 1.0f;

            // Physics Module
            if (config.physics.enabled)
            {
                particleState.velocity += config.physics.gravity * deltaTime;
                particleState.velocity *= (1.0f - config.physics.drag * deltaTime);
            }

            // Vortex Module
            if (config.vortex.enabled)
            {
                Vector3 toCenter = config.vortex.center - particleState.transform->translation_;
                if (toCenter.Length() > 0.001f)
                {
                    Vector3 toCenter_norm = toCenter.Normalize();
                    Vector3 orbitalForce = toCenter_norm * config.vortex.orbitalSpeed;
                    Vector3 rotationalForce = { -toCenter_norm.y, toCenter_norm.x, 0.0f };
                    rotationalForce = rotationalForce * config.vortex.rotationSpeed;
                    particleState.velocity += (orbitalForce + rotationalForce) * deltaTime;
                }
            }

            // Attraction Module
            if (config.attraction.enabled)
            {
                Vector3 directionToTarget = config.attraction.target - particleState.transform->translation_;
                particleState.velocity += directionToTarget.Normalize() * config.attraction.strength * deltaTime;
            }

            // 位置を更新
            particleState.transform->translation_ += particleState.velocity * deltaTime;

            // Rotation Module
            if (config.rotation.enabled)
            {
                if (config.rotation.isBillboard)
                {
                    particleState.transform->rotation_.x = 0.0f;
                    particleState.transform->rotation_.y = 0.0f;
                    particleState.transform->rotation_.z += config.rotation.angularVelocity2D * deltaTime;
                }
                else
                {
                    particleState.transform->rotation_ += config.rotation.angularVelocity3D * deltaTime;
                }
            }
            particleState.transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(particleState.transform->rotation_);

            // Color Module
            if (config.colorOverLifetime.enabled)
                particleState.color = config.colorOverLifetime.Evaluate(t);

            // Size Module
            if (config.sizeOverLifetime.enabled)
                particleState.transform->scale_ = config.sizeOverLifetime.Evaluate(t);

            // Texture Module
            particleState.textureHandle = config.textureSheet.textureHandle;

        }

        // Trail(軌跡)処理
        if (config.trail.enabled)
        {
            Vector3 currentPos = particleState.transform->translation_;

            // 新しいポイント追加は生きている間だけ
            if (!isExpired)
            {
                bool shouldAdd = false;
                if (particleState.trailHistory.empty())
                {
                    shouldAdd = true;
                }
                else 
                {
                    Vector3 lastPos = particleState.trailHistory.back().position;
                    float distSq = (currentPos - lastPos).LengthSq();
                    if (distSq >= config.trail.minVertexDistance * config.trail.minVertexDistance) 
                    {
                        shouldAdd = true;
                    }
                }

                if (shouldAdd) 
                {
                    TrailPoint newPoint;
                    newPoint.position = currentPos;
                    newPoint.rotationQuaternion = particleState.transform->rotationQuaternion_;
                    newPoint.time = particleState.age;
                    particleState.trailHistory.push_back(newPoint);
                }
            }

            // 死亡後も古い点を寿命で削除
            while (!particleState.trailHistory.empty())
            {
                float timeAlive = particleState.age - particleState.trailHistory.front().time;
                if (timeAlive > config.trail.lifetime)
                    particleState.trailHistory.pop_front();
                else
                    break;
            }
        }

        if (!isExpired) {
            particleState.transform->UpdateMatrix();
        }

        ++particle;
    }

    // GPUへ送信
    for (auto& particle : particles_)
    {
        if (particle.age >= particle.lifetime) continue;

        particle.transform->UpdateMatrix();
        engine_->renderer_->SubmitParticleInstance(
            *particle.transform,
            Math::ColorVectorToUint32(particle.color),
            particle.textureHandle,
            particle.transform->rotation_.z,
            particle.config.rotation.isBillboard
        );
    }
}

void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter> emitter)
{
    const std::string& name = emitter->presetName_;

    namedEmitters_[name] = emitter.get();

    emitters_.push_back(std::move(emitter));
}

void ParticleSystem::Draw(Camera* camera)
{
    engine_->SetBlendMode(BlendMode::kBlendModeAdd);
    engine_->renderer_->DrawParticles(*camera);

    // トレイル描画
    for (const auto& particle : particles_)
    {
        if (!particle.config.trail.enabled) continue;
        if (particle.trailHistory.size() < 2) continue;

        // ポイントリスト作成（回転情報も含める）
        std::vector<TrailPoint> drawPoints;
        drawPoints.reserve(particle.trailHistory.size() + 1);

        for (const auto& tp : particle.trailHistory) {
            drawPoints.push_back(tp);
        }

        // 生きているなら現在位置も追加
        if (particle.age < particle.lifetime)
        {
            drawPoints.push_back({
                particle.transform->translation_,
                particle.transform->rotationQuaternion_,
                particle.age
                });
        }

        // Renderer呼び出し
        engine_->renderer_->DrawTrail(
            drawPoints,
            particle.config.trail, 
            *camera
        );
    }

    engine_->SetBlendMode(BlendMode::kBlendModeNormal);

#ifdef _DEBUG
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