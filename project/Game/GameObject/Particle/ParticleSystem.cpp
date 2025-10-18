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

    for (size_t i = 0; i < static_cast<size_t>(ParticleType::Count); ++i) 
    {
        configManager_->LoadParticleDefinitionFromJson(static_cast<ParticleType>(i));
    }

    behaviors_[ParticleType::Key] = std::make_unique<KeyParticleBehavior>();
    behaviors_[ParticleType::HitEffect] = std::make_unique<HitEffectParticleBehavior>();
}

void ParticleSystem::SpawnParticle(WorldTransform& transform, ParticleType type, const std::string& presetName, float lifetime, int amount)
{
    if (particles_.size() >= engine_->kMaxParticleCount) return;

    const ParticleConfig& config = GetConfig(type, presetName);

    ParticleState particle;
    particle.transform = std::make_unique<WorldTransform>(transform);
    particle.color = config.baseColor;
    particle.type = type;
    particle.amount = amount;
    particle.textureHandle = config.textureIndex;
    particle.lifetime = lifetime;
    particle.age = 0.0f;
    particle.hasLifetime = true;
    particle.initialPosition = transform.translation_;
    particle.presetName = presetName;

    // タイプごとの初期値
    auto it = behaviors_.find(type);
    if (it != behaviors_.end()) 
    {
        it->second->Initialize(particle, config);
    }

    particles_.push_back(std::move(particle));
}

std::unique_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(ParticleType type, const std::string& presetName)
{
    // type の定義が存在しない場合、初期化（空マップ追加）
    if (definitions_.find(type) == definitions_.end()) 
    {
        definitions_[type] = {}; // 空のプリセットマップを追加
        std::cout << "No preset map found for type " << ParticleTypeToString(type) << ". Creating a new one..." << std::endl;
    }

    // presetName が存在しない場合は新規作成
    auto& presetMap = definitions_[type];
    if (presetMap.find(presetName) == presetMap.end())
    {
        std::cout << "Preset '" << presetName << "' for type " << ParticleTypeToString(type)
            << " not found. Creating a new default preset..." << std::endl;

        presetMap[presetName] = ParticleDefinition(); // デフォルトの空定義を追加
        configManager_->SaveConfigToJson(type); // 保存
    }

    // 必ず存在するはずなので、参照取得
    const auto& definition = presetMap.at(presetName);
    const auto& emitterConfig = definition.emitterConfig;

    // 新しいエミッターを生成
    auto emitter = std::make_unique<ParticleEmitter>();

    emitter->type_ = type;

    emitter->presetName_ = presetName;

    // ロードした設定で初期化
    emitter->Initialize(
        type,
        emitterConfig.position,
        emitterConfig.spawnInterval,
        emitterConfig.lifetime,
        emitterConfig.amount
    );

    return emitter;
}
 
void ParticleSystem::Update()
{
    for (auto& emitter : emitters_)
    {
        emitter->Update(*this);
    }

    // パーティクル更新処理
    for (auto particle = particles_.begin(); particle != particles_.end(); ) 
    {
        auto behavior = behaviors_.find(particle->type);
        if (behavior != behaviors_.end()) 
        {
            behavior->second->Update(*particle);
        }

        if (particle->hasLifetime)
        {
            particle->lifetime -= TimeManager::GetInstance()->GetDeltaTime();
            if (particle->lifetime <= 0.0f)
            {
                particle = particles_.erase(particle); // 寿命が尽きたパーティクルを消去
                continue;
            }
        }
        ++particle;
    }
   
    // パーティクルインスタンスの更新
    for (auto& particle : particles_)
    {
        particle.transform->UpdateMatrix();
        engine_->SubmitParticleInstance(*particle.transform, ColorVectorToUint32(particle.color), particle.textureHandle, particle.transform->rotation_.z);
    }
}

void ParticleSystem::AddEmitter(std::unique_ptr<ParticleEmitter> emitter)
{
    // 名前が指定されていない場合は自動で命名
    if (emitter->name_.empty()) 
    {
        emitter->name_ = "Emitter_" + std::string(ParticleTypeToString(emitter->type_));

        // 同じタイプが複数ある場合に備えて連番をつける
        int suffix = 1;
        std::string baseName = emitter->name_;
        while (std::any_of(emitters_.begin(), emitters_.end(), [&](const auto& e)
            {
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