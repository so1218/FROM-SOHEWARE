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

void ParticleSystem::SpawnParticle(WorldTransform& transform, const std::string& presetName, float lifetime, const WorldTransform* attractionTarget)
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
        const auto& rotConfig = particle.config.rotation;

        // 初期角度の決定
        Vector3 startRot;
        startRot.x = Math::RandomFloat(rotConfig.minStartRotation.x, rotConfig.maxStartRotation.x);
        startRot.y = Math::RandomFloat(rotConfig.minStartRotation.y, rotConfig.maxStartRotation.y);
        startRot.z = Math::RandomFloat(rotConfig.minStartRotation.z, rotConfig.maxStartRotation.z);

        // ラジアン変換してセット
        particle.transform->rotation_ = {
            Math::ToRadians(startRot.x),
            Math::ToRadians(startRot.y),
            Math::ToRadians(startRot.z)
        };

        // 回転速度の決定
        if (rotConfig.isBillboard)
        {
            // 2Dの場合、Z軸回転のみ速度を持つ
            float velZ = Math::RandomFloat(rotConfig.minAngularVelocity2D, rotConfig.maxAngularVelocity2D);
            particle.currentAngularVelocity = { 0.0f, 0.0f, velZ };
        }
        else
        {
            // 3Dの場合、3軸それぞれの速度
            particle.currentAngularVelocity.x = Math::RandomFloat(rotConfig.minAngularVelocity3D.x, rotConfig.maxAngularVelocity3D.x);
            particle.currentAngularVelocity.y = Math::RandomFloat(rotConfig.minAngularVelocity3D.y, rotConfig.maxAngularVelocity3D.y);
            particle.currentAngularVelocity.z = Math::RandomFloat(rotConfig.minAngularVelocity3D.z, rotConfig.maxAngularVelocity3D.z);
        }
    }
    else
    {
        particle.transform->rotation_ = { 0.0f, 0.0f, 0.0f };
        particle.currentAngularVelocity = { 0.0f, 0.0f, 0.0f };
    }

    // 共通プロパティ
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.presetName = presetName;
    particle.attractionTarget = attractionTarget;

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
                // 1. 中心と軸の計算
                Vector3 vortexCenter = particleState.initialPosition + config.vortex.center;
                Vector3 diff = particleState.transform->translation_ - vortexCenter;
                Vector3 axis = config.vortex.axis.Normalize();

                // 2. 軸成分と半径成分の分解
                float height = diff.Dot(axis);          // 軸方向の高さ
                Vector3 pointOnAxis = axis * height;    // 軸上の点
                Vector3 radialVector = diff - pointOnAxis; // 軸からパーティクルへのベクトル
                float distanceToAxis = radialVector.Length();

                if (distanceToAxis > 0.01f) // 0除算防止
                {
                    Vector3 radialDir = radialVector.Normalize();           // 外向き
                    Vector3 tangentialDir = axis.Cross(radialDir).Normalize(); // 接線方向(回転)

                    // 3. 速度の合成
                    // 現在の「軸方向（上昇）の速度」だけは維持する (VelocityやGravityの影響を残すため)
                    float currentAxialSpeed = particleState.velocity.Dot(axis);
                    Vector3 axialVelocity = axis * currentAxialSpeed;

                    // 設定値を使って速度を作成
                    Vector3 orbitalVelocity = tangentialDir * config.vortex.orbitalSpeed;

                    // radialSpeedが負なら中心へ (収束)、正なら外へ (拡散)
                    Vector3 radialVelocity = radialDir * config.vortex.radialSpeed;

                    // 4. 目標速度
                    Vector3 targetVelocity = axialVelocity + orbitalVelocity + radialVelocity;

                    // 5. 速度を適用 (Lerpで補間すると、少し慣性が残って自然になります)
                    // 10.0f * deltaTime くらいで強めに補間
                    float lerpRate = 10.0f * deltaTime;
                    if (lerpRate > 1.0f) lerpRate = 1.0f;

                    particleState.velocity = Math::Lerp(particleState.velocity, targetVelocity, lerpRate);
                }
            }

            // Attraction Module
            if (config.attraction.enabled)
            {
                Vector3 targetPos;

                // ターゲットが設定されていればターゲット座標 + オフセット
                if (particleState.attractionTarget)
                {
                    targetPos = particleState.attractionTarget->translation_ + config.attraction.offset;
                }
                // 設定されていなければ静的ターゲット座標
                else
                {
                    targetPos = config.attraction.target;
                }

                Vector3 directionToTarget = targetPos - particleState.transform->translation_;
                particleState.velocity += directionToTarget.Normalize() * config.attraction.strength * deltaTime;
            }

            // 位置を更新
            particleState.transform->translation_ += particleState.velocity * deltaTime;

            // Rotation Module
            if (config.rotation.enabled)
            {
                // 生成時に決定したこのパーティクル固有の回転速度を取得
                Vector3 angularVelocity = particleState.currentAngularVelocity;

                if (config.rotation.isBillboard)
                {
                    // ビルボードの場合、XY軸の回転は0に固定し、Z軸だけ回す
                    particleState.transform->rotation_.x = 0.0f;
                    particleState.transform->rotation_.y = 0.0f;

                    // 度数法で保存されている速度をラジアンに変換して加算
                    particleState.transform->rotation_.z += Math::ToRadians(angularVelocity.z) * deltaTime;
                }
                else
                {
                    // 3Dモデルの場合、XYZ全軸を回転させる
                    Vector3 velocityRadians = 
                    {
                        Math::ToRadians(angularVelocity.x),
                        Math::ToRadians(angularVelocity.y),
                        Math::ToRadians(angularVelocity.z)
                    };

                    particleState.transform->rotation_ += velocityRadians * deltaTime;
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
            particle.config.blendMode,
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
    engine_->renderer_->DrawParticles(*camera);

    engine_->SetBlendMode(BlendMode::kBlendModeAdd);
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