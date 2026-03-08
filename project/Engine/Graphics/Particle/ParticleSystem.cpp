#include "pch.h"
#define _USE_MATH_DEFINES

#include "ParticleSystem.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "ParticleEditor.h"
#include "ParticleConfigManager.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "json.hpp"

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
}

void ParticleSystem::SpawnParticle(const WorldTransform& transform, const std::string& presetName, float lifetime, const WorldTransform* attractionTarget, const WorldTransform* vortexTarget
    , const ModelData* emitterModelData, const AnimationModel* emitterAnimModel)
{
    // 最大数を超える場合は生成しない
    if (particles_.size() >= engine_->GetRendererManager()->GetMaxParticleCount()) return;

    // パーティクル設定を取得
    auto& config = GetConfig(presetName);

    // 見えない状態かどうかのフラグ
    uint32_t startA = (config.colorOverLifetime.startColor) & 0xFF;
    uint32_t endA = (config.colorOverLifetime.endColor) & 0xFF;
    bool invisibleAlpha = (startA < 5 && endA < 5);

    bool startZero = (config.sizeOverLifetime.startScale.x <= 0.0f || config.sizeOverLifetime.startScale.y <= 0.0f);
    bool endZero = (config.sizeOverLifetime.endScale.x <= 0.0f || config.sizeOverLifetime.endScale.y <= 0.0f);
    bool invisibleScale = (startZero && endZero);

    // トレイルが無効かつアルファかサイズで見えない状態の時だけ生成をキャンセル
    if (!config.trail.enabled && (invisibleAlpha || invisibleScale))
    {
        return;
    }

    ParticleState particle;
    particle.config = config;

    // メモリを割り当てる
    particle.transform = std::make_unique<WorldTransform>();

    // Shape
    Vector3 localOffset = particle.config.shape.GetInitialPositionOffset(emitterModelData, emitterAnimModel);

    // 行列の作成
    Matrix4x4 transformMatrix = Matrix4x4::MakeAffine(
        transform.scale_,
        transform.rotationQuaternion_,
        { 0.0f, 0.0f, 0.0f }
    );

    // ローカルオフセットを行列で変換（ワールド空間でのオフセット）
    Vector3 worldOffset = transformMatrix.TransformVector(localOffset);

    // 最終的なワールド座標を適用
    particle.transform->translation_ = transform.translation_ + worldOffset;

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

    // 初期色
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
    particle.vortexTarget = vortexTarget;
    particle.trailSeed = Math::RandomFloat(0.0f, 1000.0f);

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

            if (config.noise.enabled)
            {
                // ノイズのサンプリング座標を計算
                float frequency = config.noise.frequency;
                float scroll = TimeManager::GetInstance()->GetTotalTime() * config.noise.scrollSpeed;

                Vector3 samplePos = particleState.transform->translation_ * frequency;

                // Y軸方向へスクロール
                samplePos.y -= scroll;

                // ノイズ強度の計算
                float strength = config.noise.strength;

                // 3軸分のノイズを計算              
                Vector3 noiseVelocity;

                if (config.noise.separateAxes)
                {
                    noiseVelocity.x = Math::PerlinNoise(samplePos.x, samplePos.y, samplePos.z);
                    noiseVelocity.y = Math::PerlinNoise(samplePos.x, samplePos.y + 100.0f, samplePos.z);
                    noiseVelocity.z = Math::PerlinNoise(samplePos.x, samplePos.y + 200.0f, samplePos.z);
                }
                else
                {
                    noiseVelocity.x = Math::PerlinNoise(samplePos.x, samplePos.y, samplePos.z);
                    noiseVelocity.y = Math::PerlinNoise(samplePos.x, samplePos.y + 100.0f, samplePos.z);
                    noiseVelocity.z = Math::PerlinNoise(samplePos.x, samplePos.y + 200.0f, samplePos.z);
                }

                // 速度に加算
                particleState.transform->translation_ += noiseVelocity * strength * deltaTime;
            }

            // Physics Module
            if (config.physics.enabled)
            {
                particleState.velocity += config.physics.gravity * deltaTime;
                particleState.velocity *= (1.0f - config.physics.drag * deltaTime);
            }

            if (config.collision.enabled)
            {
                Vector3& pos = particleState.transform->translation_;
                Vector3& vel = particleState.velocity;
                const auto& col = config.collision;

                bool isCollided = false;
                Vector3 normal = { 0.0f, 1.0f, 0.0f };
                float penetration = 0.0f; // めり込み量

                // 平面衝突
                if (col.type == CollisionModule::Type::Plane)
                {
                    Vector3 vecToParticle = pos - col.plane.point;
                    float dist = vecToParticle.Dot(col.plane.normal);

                    // 平面の裏側(距離が負)に行ったら衝突
                    if (dist < 0.0f)
                    {
                        isCollided = true;
                        normal = col.plane.normal;
                        penetration = -dist;
                    }
                }
                // 簡易ワールド衝突
                else if (col.type == CollisionModule::Type::World)
                {
                    if (col.worldObj.shape == CollisionModule::WorldObject::Shape::Sphere)
                    {
                        Vector3 diff = pos - col.worldObj.center;
                        float distSq = diff.LengthSq();
                        float r = col.worldObj.scale.x; 

                        if (distSq < r * r)
                        {
                            isCollided = true;
                            float dist = sqrtf(distSq);
                            if (dist > 0.0001f) {
                                normal = diff / dist;
                                penetration = r - dist;
                            }
                        }
                    }
                    else if (col.worldObj.shape == CollisionModule::WorldObject::Shape::Box)
                    {
                        // AABB判定
                        Vector3 halfSize = col.worldObj.scale * 0.5f;
                        Vector3 min = col.worldObj.center - halfSize;
                        Vector3 max = col.worldObj.center + halfSize;

                        if (pos.x > min.x && pos.x < max.x &&
                            pos.y > min.y && pos.y < max.y &&
                            pos.z > min.z && pos.z < max.z)
                        {
                            isCollided = true;
                            // 最も近い面を探して法線を決定
                            normal = { 0.0f, 1.0f, 0.0f };
                            penetration = (col.worldObj.center.y + halfSize.y) - pos.y;
                        }
                    }
                }

                // 衝突時の応答処理
                if (isCollided)
                {
                    // 位置補正
                    pos += normal * penetration;

                    // 速度の反射と減衰
                    float dot = vel.Dot(normal);
                    if (dot < 0.0f) // 面に向かって進んでいる時のみ
                    {
                        Vector3 normalVel = normal * dot;
                        Vector3 tangentVel = vel - normalVel;

                        // 反発 
                        normalVel = normalVel * -col.bounce;

                        // 摩擦 
                        tangentVel = tangentVel * (1.0f - col.friction);

                        // 合成
                        vel = normalVel + tangentVel;

                        // 全体的なエネルギー減衰
                        vel *= (1.0f - col.dampen);
                    }

                    // 寿命減少
                    particleState.age += particleState.lifetime * col.lifeLoss;
                    if (particleState.age >= particleState.lifetime) {}
                }
            }

            // Vortex Module
            if (config.vortex.enabled)
            {
                Vector3 vortexCenter;

                if (particleState.vortexTarget != nullptr)
                {
                    vortexCenter = particleState.vortexTarget->translation_ + config.vortex.offset;
                }
                else
                {
                    vortexCenter = particleState.initialPosition + config.vortex.center;
                }

                Vector3 diff = particleState.transform->translation_ - vortexCenter;
                Vector3 axis = config.vortex.axis.Normalize();

                float height = diff.Dot(axis);
                Vector3 pointOnAxis = axis * height;
                Vector3 radialVector = diff - pointOnAxis;
                float distanceToAxis = radialVector.Length();

                if (distanceToAxis > 0.01f)
                {
                    Vector3 radialDir = radialVector.Normalize();
                    Vector3 tangentialDir = axis.Cross(radialDir).Normalize();

                    float currentAxialSpeed = particleState.velocity.Dot(axis);
                    Vector3 axialVelocity = axis * currentAxialSpeed;

                    Vector3 orbitalVelocity = tangentialDir * config.vortex.orbitalSpeed;

                    Vector3 radialVelocity = radialDir * config.vortex.radialSpeed;

                    Vector3 targetVelocity = axialVelocity + orbitalVelocity + radialVelocity;

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
                    // 3Dモデルの場合、XYZ全軸を回転
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
            particleState.textureHandle = TextureManager::GetInstance().Get(config.textureSheet.textureName);

        }

        // Trail処理
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

        // 描画不可ならスキップ
        if (ShouldSkipDraw(particle)) continue;

        particle.transform->UpdateMatrix();
        engine_->GetRendererManager()->SubmitParticleInstance(
            *particle.transform,
            Math::ColorVectorToUint32(particle.color),
            particle.textureHandle,
            particle.transform->rotation_.z,
            particle.config.blendMode,
            particle.config.rotation.isBillboard,
            particle.config.intensity
        );
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
        if (!particle.config.trail.enabled) continue; // トレイル無効ならスキップ
        if (particle.trailHistory.size() < 2) continue; // 頂点が足りなければスキップ

        // 描画用ポイントリストを作成
        std::vector<TrailPoint> drawPoints;
        drawPoints.reserve(particle.trailHistory.size() + 1);

        for (const auto& tp : particle.trailHistory)
            drawPoints.push_back(tp);

        // 生存中なら現在位置も追加
        if (particle.age < particle.lifetime)
            drawPoints.push_back({ particle.transform->translation_, particle.transform->rotationQuaternion_, particle.age });

        // Rendererに登録
        engine_->GetRendererManager()->SubmitTrail(drawPoints, particle.config.trail, particle.trailSeed);
    }

    // ブレンドを元に戻す
    engine_->SetBlendMode(BlendMode::kBlendModeNormal);

#ifdef IS_DEVELOPMENT
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

bool ParticleSystem::ShouldSkipDraw(const ParticleState& particle) const
{
    const auto& config = particle.config;

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