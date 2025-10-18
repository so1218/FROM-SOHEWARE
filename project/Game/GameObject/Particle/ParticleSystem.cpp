#define _USE_MATH_DEFINES

#include "ParticleSystem.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "ParticleEditor.h"
#include "ParticleConfigManager.h"
#include "TextureHandle.h"
#include "KeyParticleBehavior.h"
#include "HitEffectParticleBehavior.h"
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

void ParticleSystem::SpawnParticle(WorldTransform& transform, const std::string& presetName, float lifetime, int amount)
{
    if (particles_.size() >= engine_->kMaxParticleCount) return;

    const ParticleConfig& config = GetConfig(presetName);

    ParticleState particle;
    particle.config = config;

    // --- モジュールに基づいて初期値を設定 ---

      // Shape: Emitterの座標にShapeのオフセットを加算
    particle.transform = std::make_unique<WorldTransform>();
    particle.transform->translation_ = transform.translation_ + config.shape.GetInitialPositionOffset();

    // Velocity: 初速を決定
    if (config.velocity.enabled) {
        particle.velocity = config.velocity.GetInitialVelocity();
    }

    // Color: 開始色を設定 (t=0の時の色)
    if (config.colorOverLifetime.enabled) {
        particle.color = config.colorOverLifetime.Evaluate(0.0f);
    }
    else {
        particle.color = config.baseColor; // モジュール無効なら基本色
    }

    // Size: 開始スケールを設定 (t=0の時のスケール)
    if (config.sizeOverLifetime.enabled) {
        particle.transform->scale_ = config.sizeOverLifetime.Evaluate(0.0f);
    }
    else {
        particle.transform->scale_ = { 1.0f, 1.0f, 1.0f }; // デフォルト値
    }

    // Rotation: 初期角度を設定
    if (config.rotation.enabled && config.rotation.randomStartRotation) {
        particle.transform->rotation_.z = RandomFloat(0.0f, 360.0f);
    }

    // --- 基本的なプロパティを設定 ---
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.presetName = presetName;
    particle.isExist = true; // 新しいシステムではこのフラグは不要になるかも

    particles_.push_back(std::move(particle));
}

// presetNameだけでエミッターを生成する
std::unique_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(const std::string& presetName)
{
    // definitions_ マップにプリセットが存在するかチェック
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
    emitter->Initialize(
        emitterConfig.position,
        emitterConfig.spawnInterval,
        emitterConfig.lifetime,
        emitterConfig.amount
    );

    return emitter;
}
 
void ParticleSystem::Update()
{
    // 1. エミッターを更新して、新しいパーティクルを生成させる
    for (auto& emitter : emitters_)
    {
        emitter->Update(*this);
    }

    // 2. フレームの経過時間を取得
    float dt = TimeManager::GetInstance()->GetDeltaTime();

    // 3. 全パーティクルを更新する汎用ループ
    for (auto it = particles_.begin(); it != particles_.end(); )
    {
        ParticleState& p = *it;
        const ParticleConfig& config = p.config; // パーティクルの設定を参照

        // --- 寿命の処理 ---
        p.age += dt;
        if (p.age >= p.lifetime) {
            it = particles_.erase(it); // 寿命が尽きたら消去
            continue;
        }
        // 正規化された寿命 (0.0～1.0) を計算
        float t = p.lifetime > 0.0f ? (p.age / p.lifetime) : 1.0f;

        // --- ここからモジュールごとの処理 ---

        // Physics Module: 速度を更新
        if (config.physics.enabled) {
            p.velocity.y -= config.physics.gravity * dt;
            p.velocity = p.velocity * (1.0f - (config.physics.drag * dt));
        }

        // 移動: 速度を位置に反映
        p.transform->translation_ += p.velocity * dt;

        // Rotation Module: 回転を更新
        if (config.rotation.enabled) {
            p.transform->rotation_.z += config.rotation.angularVelocity * dt;
        }
        p.transform->rotationQuaternion_ = Quaternion::QuaternionFromEuler(p.transform->rotation_);

        // ColorOverLifetime Module: 色を更新
        if (config.colorOverLifetime.enabled) {
            p.color = config.colorOverLifetime.Evaluate(t);
        }

        // SizeOverLifetime Module: スケールを更新
        if (config.sizeOverLifetime.enabled) {
            p.transform->scale_ = config.sizeOverLifetime.Evaluate(t);
        }

        // TextureSheetAnimation Module: テクスチャのUVを更新
        if (config.textureSheet.enabled) {
            p.textureHandle = config.textureSheet.textureHandle;
            // (ここにUV座標を計算するロジックを実装)
        }
        else {
            p.textureHandle = config.textureSheet.textureHandle;
            // (UVはデフォルト値(全面)を使用)
        }

        ++it;
    }

    // 4. 全パーティクルのインスタンス情報をGPUに送る
    for (auto& particle : particles_)
    {
        particle.transform->UpdateMatrix();
        engine_->SubmitParticleInstance(
            *particle.transform,
            ColorVectorToUint32(particle.color),
            particle.textureHandle,
            particle.transform->rotation_.z
        );
    }
}

void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter> emitter)
{
    // 名前が指定されていない場合は自動で命名
    if (emitter->name_.empty())
    {
        // presetNameをベースに名前を付ける
        emitter->name_ = "Emitter_" + emitter->presetName_;

        // 同じ名前が複数ある場合に備えて連番をつける
        int suffix = 1;
        std::string baseName = emitter->name_;
        while (std::any_of(emitters_.begin(), emitters_.end(), [&](const auto& e) {
            return e->name_ == emitter->name_;
            }))
        {
            emitter->name_ = baseName + "_" + std::to_string(suffix++);
        }
    }

    emitters_.push_back(std::move(emitter));
}

void ParticleSystem::Draw()
{
    editor_->ShowEditor();
}