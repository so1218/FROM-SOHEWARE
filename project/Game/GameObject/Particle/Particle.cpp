#define _USE_MATH_DEFINES

#include "Particle.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "ParticleEditor.h"
#include "TextureHandle.h"
#include "KeyParticleBehavior.h"
#include "HitEffectParticleBehavior.h"
#include "ImGuiManager.h"
#include "json.hpp"



ParticleSystem::ParticleSystem()
{
    editor_ = std::make_unique<ParticleEditor>(this);
}

ParticleSystem::~ParticleSystem() = default;

void ParticleSystem::Initialize(Engine* engine)
{
    engine_ = engine;

    for (size_t i = 0; i < static_cast<size_t>(ParticleType::Count); ++i) 
    {
        LoadParticleDefinitionFromJson(static_cast<ParticleType>(i));
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

void ParticleSystem::LoadParticleDefinitionFromJson(ParticleType type)
{
    std::string typeName = ParticleTypeToString(type);
    std::string filepath = kConfigDirectoryPath_ + typeName + "Particles.json";

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open particle config file: " << filepath << std::endl;
        return;
    }

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        std::cerr << "Error parsing JSON (" << filepath << "): " << e.what() << std::endl;
        return;
    }

    if (!j.contains(typeName))
    {
        std::cerr << "JSON does not contain key: " << typeName << std::endl;
        return;
    }

    for (auto& [presetName, presetJson] : j[typeName].items())
    {
        // 新しいマップから対象の定義を取得（または新規作成）
        auto& definition = definitions_[type][presetName];

        // ParticleConfigの読み込み
        if (presetJson.contains("ParticleConfig")) {
            auto& configJson = presetJson["ParticleConfig"];
            auto& config = definition.particleConfig; // ParticleConfigへの参照を取得

            config.type = type;
            config.speed = configJson.value("speed", 0.0f);
            config.gravity = configJson.value("gravity", 0.0f);
            config.drag = configJson.value("drag", 0.0f);
            config.decayRate = configJson.value("decayRate", 1.0f);
            config.maxLifetime = configJson.value("maxLifetime", 5.0f);
            config.textureIndex = configJson.value("textureIndex", 0);
            config.radius = configJson.value("particleRadius", 1.0f);

            if (configJson.contains("baseColor") && configJson["baseColor"].is_array()) {
                auto colorArray = configJson["baseColor"];
                config.baseColor = {
                    colorArray[0].get<float>(),
                    colorArray[1].get<float>(),
                    colorArray[2].get<float>(),
                    colorArray[3].get<float>()
                };
            }

            if (configJson.contains("emitterRange") && configJson["emitterRange"].is_array()) {
                auto& r = configJson["emitterRange"];
                config.emitterRange = {
                    r[0].get<float>(),
                    r[1].get<float>(),
                    r[2].get<float>()
                };
            }

            config.fadeOutEase.frameCount_ = configJson.value("fadeOutFrameCount", 60);
            config.startColor = configJson.value("startColor", 0xffffffff);
            config.endColor = configJson.value("endColor", 0xffffffff);
            config.scaleEase.frameCount_ = configJson.value("scaleFrameCount", 60);
            if (configJson.contains("startScale") && configJson["startScale"].is_array()) {
                auto& arr = configJson["startScale"];
                config.startScale = {
                    arr[0].get<float>(),
                    arr[1].get<float>(),
                    arr[2].get<float>()
                };
            }

            if (configJson.contains("endScale") && configJson["endScale"].is_array()) {
                auto& arr = configJson["endScale"];
                config.endScale = {
                    arr[0].get<float>(),
                    arr[1].get<float>(),
                    arr[2].get<float>()
                };
            }
        }

        // EmitterConfigの読み込み
        if (presetJson.contains("EmitterConfig")) {
            auto& emitterJson = presetJson["EmitterConfig"];
            auto& emitterConfig = definition.emitterConfig; // EmitterConfigへの参照を取得

            if (emitterJson.contains("position") && emitterJson["position"].is_array()) {
                emitterConfig.position = {
                    emitterJson["position"][0].get<float>(),
                    emitterJson["position"][1].get<float>(),
                    emitterJson["position"][2].get<float>()
                };
            }
            emitterConfig.spawnInterval = emitterJson.value("spawnInterval", 0.1f);
            emitterConfig.lifetime = emitterJson.value("lifetime", 5.0f);
            emitterConfig.amount = emitterJson.value("amount", 1);
        }
    }
}

std::unique_ptr<ParticleEmitter> ParticleSystem::CreateEmitter(ParticleType type, const std::string& presetName)
{
    // type の定義が存在しない場合、初期化（空マップ追加）
    if (definitions_.find(type) == definitions_.end()) {
        definitions_[type] = {}; // 空のプリセットマップを追加
        std::cout << "No preset map found for type " << ParticleTypeToString(type) << ". Creating a new one..." << std::endl;
    }

    // presetName が存在しない場合は新規作成
    auto& presetMap = definitions_[type];
    if (presetMap.find(presetName) == presetMap.end()) {
        std::cout << "Preset '" << presetName << "' for type " << ParticleTypeToString(type)
            << " not found. Creating a new default preset..." << std::endl;

        presetMap[presetName] = ParticleDefinition(); // デフォルトの空定義を追加
        SaveConfigToJson(type); // 保存
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
    for (auto it = particles_.begin(); it != particles_.end(); ) 
    {
        auto behavior = behaviors_.find(it->type);
        if (behavior != behaviors_.end()) 
        {
            behavior->second->Update(*it);
        }

        if (it->hasLifetime)
        {
            it->lifetime -= TimeManager::GetInstance()->GetDeltaTime();
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

void ParticleSystem::SaveConfigToJson(ParticleType type) 
{
    // 指定されたタイプのプリセットマップを取得
    const auto& presets = definitions_.at(type);
    std::string typeName = ParticleTypeToString(type);

    nlohmann::json rootJson;
    nlohmann::json typeJson;


    // そのタイプの全プリセットをループしてJSONオブジェクトを構築
    for (const auto& [presetName, definition] : presets)
    {
        const auto& config = definition.particleConfig;
        const auto& emitterConfig = definition.emitterConfig;

        // ParticleConfigをJSONに変換
        nlohmann::json particleConfigJson = {
            { "speed", config.speed },
            { "gravity", config.gravity },
            { "drag", config.drag },
            { "decayRate", config.decayRate },
            { "maxLifetime", config.maxLifetime },
            { "textureIndex", config.textureIndex },
            { "particleRadius", config.radius },
            { "baseColor", { config.baseColor.x, config.baseColor.y, config.baseColor.z, config.baseColor.w }},
            { "emitterRange", { config.emitterRange.x, config.emitterRange.y, config.emitterRange.z }},
            { "fadeOutFrameCount", config.fadeOutEase.frameCount_ },
            { "startColor", config.startColor },
            { "endColor", config.endColor },
            { "scaleFrameCount", config.scaleEase.frameCount_ },
            { "startScale", { config.startScale.x, config.startScale.y, config.startScale.z }},
            { "endScale", { config.endScale.x, config.endScale.y, config.endScale.z }},
        };

        // EmitterConfigをJSONに変換
        nlohmann::json emitterConfigJson = {
            { "position", { emitterConfig.position.x, emitterConfig.position.y, emitterConfig.position.z }},
            { "spawnInterval", emitterConfig.spawnInterval },
            { "lifetime", emitterConfig.lifetime },
            { "amount", emitterConfig.amount }
        };

        // プリセット名をキーとしてJSONを構築
        typeJson[presetName] = {
            { "ParticleConfig", particleConfigJson },
            { "EmitterConfig", emitterConfigJson }
        };
    }

    // 最終的なJSONオブジェクトを構築
    rootJson[typeName] = typeJson;

    std::string filename = kConfigDirectoryPath_ + typeName + "Particles.json";
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // dump(4) でインデントを付けて見やすく出力
    ofs << rootJson.dump(4);
}

void ParticleSystem::ApplyEmitterConfigToLiveEmitters(ParticleType type, const std::string& presetName)
{
    // 更新する設定（設計図）を type と presetName の両方で特定する
    const auto& emitterConfig = definitions_.at(type).at(presetName).emitterConfig;

    // 全てのライブエミッターをループ
    for (auto& emitter : emitters_) {
        // タイプとプリセット名の両方が一致するエミッターを見つける
        if (emitter->type_ == type && emitter->presetName_ == presetName) {
            // インスタンスの値を設計図の値で上書きする
            emitter->position_ = emitterConfig.position;
            emitter->spawnInterval_ = emitterConfig.spawnInterval;
            emitter->lifetime_ = emitterConfig.lifetime;
            emitter->amount_ = emitterConfig.amount;
        }
    }
}