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
    editor_ = std::make_unique<ParticleEditor>(this);
    configManager_ = std::make_unique<ParticleConfigManager>(this);
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Initialize(Engine* engine)
{
    engine_ = engine;

    configManager_->LoadAllParticleDefinitions();
}

void ParticleSystem::SpawnParticle(WorldTransform& transform, const std::string& presetName, float lifetime)
{
    if (particles_.size() >= engine_->renderer_->kMaxParticleCount) return;

    const ParticleConfig& config = GetConfig(presetName);

    ParticleState particle;
    particle.config = config;

    // モジュールに基づいて初期値を設定

    // Shape: Emitterの座標にShapeのオフセットを加算
    particle.transform = std::make_unique<WorldTransform>();
    particle.transform->translation_ = transform.translation_ + config.shape.GetInitialPositionOffset();

    // Velocity: 初期設定
    if (config.velocity.enabled)
    {
        particle.velocity = config.velocity.GetInitialVelocity();
    }

    // Color: 初期設定
    if (config.colorOverLifetime.enabled)
    {
        particle.color = config.colorOverLifetime.Evaluate(0.0f);
    }
    else
    {
        particle.color = config.baseColor; // モジュール無効なら基本色
    }

    // Size: 初期設定
    if (config.sizeOverLifetime.enabled)
    {
        particle.transform->scale_ = config.sizeOverLifetime.Evaluate(0.0f);
    }
    else
    {
        particle.transform->scale_ = { 1.0f, 1.0f, 1.0f };
    }

    // Rotation: 初期設定
    if (config.rotation.enabled)
    {
        if (config.rotation.isBillboard)
        {
            // ビルボードが有効で、かつランダムな初期回転が設定されている場合
            if (config.rotation.randomStartRotation)
            {
                // Z軸にランダムな初期回転を設定
                particle.transform->rotation_.z = RandomFloat(0.0f, 360.0f);
            }
            else
            {
                // orientation3D.z を 2D の初期回転として使用する
                particle.transform->rotation_.z = ToRadians(config.rotation.orientation3D.z);
            }
        }
        else
        {
            // ビルボードが無効な場合、設定された向きをそのまま適用
            particle.transform->rotation_.x = ToRadians(config.rotation.orientation3D.x);
            particle.transform->rotation_.y = ToRadians(config.rotation.orientation3D.y);
            particle.transform->rotation_.z = ToRadians(config.rotation.orientation3D.z);
        }
    }
    else
    {
        particle.transform->rotation_ = { 0.0f, 0.0f, 0.0f };
    }

    // 基本的なプロパティを設定
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.presetName = presetName;

    particles_.push_back(std::move(particle));
}

// presetNameでエミッターを生成する
std::unique_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(const std::string& presetName)
{
    // definitions_マップにプリセットが存在するかチェック
    if (definitions_.find(presetName) == definitions_.end())
    {
        // 存在しない場合は、新しいデフォルト定義を作成して保存
        definitions_[presetName] = ParticleDefinition();
        configManager_->SaveParticleDefinitionToJson(presetName); // 新しい保存関数
    }

    const auto& definition = definitions_.at(presetName);
    const auto& emitterConfig = definition.emitterConfig;

    auto emitter = std::make_unique<ParticleEmitter>();
    emitter->presetName_ = presetName; // プリセット名を保持
    emitter->Initialize(emitterConfig);

    return emitter;
}

void ParticleSystem::Update()
{
    // エミッターを更新して、新しいパーティクルを生成
    auto it = emitters_.begin();
    while (it != emitters_.end())
    {
        auto& emitter = *it;
        if (emitter->isDead_)
        {
            std::string name = emitter->presetName_;

            it = emitters_.erase(it);

            auto map_it = namedEmitters_.find(name);

            if (map_it != namedEmitters_.end())
            {
                namedEmitters_.erase(map_it);
            }

            continue;
        }
        emitter->Update(*this);
        ++it;
    }

    // フレームの経過時間を取得
    float deltaTime = TimeManager::GetInstance()->GetDeltaTime();

    // 全パーティクルを更新
    for (auto partilce = particles_.begin(); partilce != particles_.end(); )
    {
        ParticleState& particleState = *partilce;
        const ParticleConfig& config = particleState.config; // パーティクルの設定を参照

        // 寿命の処理
        particleState.age += deltaTime;
        if (particleState.age >= particleState.lifetime)
        {
            partilce = particles_.erase(partilce); // 寿命が尽きたら消去
            continue;
        }
        // 正規化された寿命を計算
        float t = particleState.lifetime > 0.0f ? (particleState.age / particleState.lifetime) : 1.0f;

        // モジュールごとの処理

        // Physics Module: 速度を更新
        if (config.physics.enabled)
        {
            particleState.velocity += config.physics.gravity * deltaTime;
            particleState.velocity = particleState.velocity * (1.0f - (config.physics.drag * deltaTime));
        }

        if (config.vortex.enabled)
        {
            // パーティクルから渦の中心へ向かうベクトルを計算
            Vector3 toCenter = config.vortex.center - particleState.transform->translation_;

            // 距離がゼロに近い場合は何もしない
            if (toCenter.Length() > 0.001f) {
                Vector3 toCenter_norm = toCenter.Normalize();

                // 中心へ向かう/離れる力（公転速度）を計算
                Vector3 orbitalForce = toCenter_norm * config.vortex.orbitalSpeed;

                // 回転方向のベクトルを計算 (2D/XY平面の場合)
                Vector3 rotationalForce = { -toCenter_norm.y, toCenter_norm.x, 0.0f };
                rotationalForce = rotationalForce * config.vortex.rotationSpeed;

                // 2つの力をパーティクルの速度に加える
                particleState.velocity += (orbitalForce + rotationalForce) * deltaTime;
            }
        }

        // Attraction Module: 引力を速度に加える
        if (config.attraction.enabled)
        {
            // パーティクルから目標への方向ベクトルを計算
            Vector3 directionToTarget = config.attraction.target - particleState.transform->translation_;

            // 正規化して、純粋な方向だけを取り出す
            Vector3 normalizedDir = directionToTarget.Normalize();

            // 速度に加えるべき力（加速度）を計算
            Vector3 attractionForce = normalizedDir * config.attraction.strength;

            // パーティクルの速度に、経過時間を考慮した力を加える
            particleState.velocity += attractionForce * deltaTime;
        }

        // 移動: 速度を位置に反映
        particleState.transform->translation_ += particleState.velocity * deltaTime;

        // Rotation Module: 回転を更新
        if (config.rotation.enabled)
        {
            if (config.rotation.isBillboard)
            {
                // ビルボード有効時
                particleState.transform->rotation_.x = 0.0f;
                particleState.transform->rotation_.y = 0.0f;
                particleState.transform->rotation_.z += config.rotation.angularVelocity2D * deltaTime;
            }
            else
            {
                // ビルボード無効時
                particleState.transform->rotation_ += config.rotation.angularVelocity3D * deltaTime;
            }
        }
        particleState.transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(particleState.transform->rotation_);

        // ColorOverLifetime Module: 色を更新
        if (config.colorOverLifetime.enabled)
        {
            particleState.color = config.colorOverLifetime.Evaluate(t);
        }

        // SizeOverLifetime Module: スケールを更新
        if (config.sizeOverLifetime.enabled)
        {
            particleState.transform->scale_ = config.sizeOverLifetime.Evaluate(t);
        }

        // TextureSheetAnimation Module: テクスチャのUVを更新
        if (config.textureSheet.enabled)
        {
            particleState.textureHandle = config.textureSheet.textureHandle;
            // UV座標を計算
        }
        else
        {
            particleState.textureHandle = config.textureSheet.textureHandle;
            // UVはデフォルト値
        }

        ++partilce;
    }

    // 全パーティクルのインスタンス情報をGPUに送る
    for (auto& particle : particles_)
    {
        particle.transform->UpdateMatrix();
        engine_->renderer_->SubmitParticleInstance(
            *particle.transform,
            ColorVectorToUint32(particle.color),
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