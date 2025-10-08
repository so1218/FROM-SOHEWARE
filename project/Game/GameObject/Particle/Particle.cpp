#define _USE_MATH_DEFINES

#include "Particle.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "TextureHandle.h"
#include "KeyParticleBehavior.h"
#include "ImGuiManager.h"
#include "json.hpp"

namespace 
{
    static const std::vector<std::pair<const char*, TextureID>> particleTextureList = 
    {
        { "white1x1", TextureID::white1x1 },
        { "uvChecker", TextureID::uvChecker },
        { "particlePurple", TextureID::particlePurple },
    };
}

void ParticleSystem::Initialize(Engine* engine)
{
    engine_ = engine;

    // JSONから読み込む
    LoadParticleDefinitionsFromJson("Game/Data/particles.json");

    // JSONに色情報がない場合、ここでデフォルト色を設定する
    particleConfigs_[static_cast<size_t>(ParticleType::None)].baseColor = Uint32ToColorVector(0xFFFFFFff);

    behaviors_[ParticleType::Key] = std::make_unique<KeyParticleBehavior>();
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
    auto it = behaviors_.find(type);
    if (it != behaviors_.end()) 
    {
        it->second->Initialize(particle, *this);
    }

    particles_.push_back(std::move(particle));
}

void ParticleSystem::LoadParticleDefinitionsFromJson(const std::string& filepath)
{

    std::ifstream file(filepath);
    nlohmann::json j;

    if (file.is_open()) {
        try {
            file >> j;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }
    }

    // 全ParticleTypeをチェック
    for (size_t i = 0; i < static_cast<size_t>(ParticleType::Count); ++i) {
        ParticleType type = static_cast<ParticleType>(i);
        std::string typeName = ParticleTypeToString(type);

        if (j.contains(typeName)) {
            auto& entry = j[typeName];
            auto& config = particleConfigs_[i];
            config.type = type;
            config.speed = entry.value("speed", 0.0f);
            config.gravity = entry.value("gravity", 0.0f);
            config.drag = entry.value("drag", 0.0f);
            config.decayRate = entry.value("decayRate", 1.0f);
            config.maxLifetime = entry.value("maxLifetime", 5.0f);
            config.textureIndex = entry.value("textureIndex", 0);
            config.radius = entry.value("particleRadius", 1.0f);

            if (entry.contains("baseColor") && entry["baseColor"].is_array()) {
                auto colorArray = entry["baseColor"];
                config.baseColor = {
                    colorArray[0].get<float>(),
                    colorArray[1].get<float>(),
                    colorArray[2].get<float>(),
                    colorArray[3].get<float>()
                };
            }

            if (entry.contains("emitterRange") && entry["emitterRange"].is_array()) {
                auto& r = entry["emitterRange"];
                config.emitterRange = {
                    r[0].get<float>(),
                    r[1].get<float>(),
                    r[2].get<float>()
                };
            }

            config.fadeOutEase->frameCount_ = entry.value("fadeOutFrameCount", 0.01f);
            config.scaleEase->frameCount_ = entry.value("scaleFrameCount", 0.04f);
            config.startColor = entry.value("startColor", 0xffffffff);
            config.endColor = entry.value("endColor", 0xffffffff);

            
        }
        else
        {
            // 未定義ならデフォルト設定で初期化
            particleConfigs_[i] = ParticleConfig{};
            particleConfigs_[i].type = type;
        }
    }
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

void ParticleSystem::AddEmitter(ParticleEmitter* emitter)
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

    emitters_.push_back(emitter);
}
void ParticleSystem::ShowEditor()
{
    if (ImGui::Begin("パーティクルエディター"))
    {
        static int selectedType = static_cast<int>(ParticleType::Key);
        ImGui::Combo("Particle Type", &selectedType, "None\0Key\0HitEffect\0\0");

        ParticleType type = static_cast<ParticleType>(selectedType);
        auto& config = GetConfig(type);

        if (ImGui::CollapsingHeader("Emitters"))
        {
            int emitterIndex = 0;
            for (auto& emitter : emitters_)
            {
                std::string label = "Emitter_" + std::string(ParticleTypeToString(emitter->type_));
                if (ImGui::TreeNode(label.c_str())) 
                {
                    ImGui::DragFloat3("Position", &emitter->position_.x, 0.1f);
                    ImGui::DragFloat("Spawn Interval", &emitter->spawnInterval_, 0.01f, 0.01f, 10.0f);
                    ImGui::DragFloat("Lifetime", &emitter->lifetime_, 0.01f, 0.0f, 10.0f);
                    ImGui::DragInt("Amount", &emitter->amount_, 1, 1, 100);

                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::CollapsingHeader("Config"))
        {
            ImGui::DragFloat("Speed", &config.speed, 0.01f);
            ImGui::DragFloat("Gravity", &config.gravity, 0.01f);

            int selectedTextureIdx = 0;
            for (size_t i = 0; i < particleTextureList.size(); ++i) {
                if (TextureHandle::Get(particleTextureList[i].second) == config.textureIndex) {
                    selectedTextureIdx = static_cast<int>(i);
                    break;
                }
            }

            // 名前配列だけ作る（Comboで使う）
            std::vector<const char*> textureNameArray;
            for (const auto& pair : particleTextureList) {
                textureNameArray.push_back(pair.first);
            }

            // Combo UI
            if (ImGui::Combo("Texture", &selectedTextureIdx, textureNameArray.data(), static_cast<int>(textureNameArray.size()))) {
                config.textureIndex = TextureHandle::Get(particleTextureList[selectedTextureIdx].second);
            }
            ImGui::ColorEdit4("Base Color", &config.baseColor.x);
            // emitterRange
            ImGui::DragFloat3("Emitter Range", &config.emitterRange.x, 0.01f);

            // fadeOutEase interval
            ImGui::DragInt("FadeOut FrameCount", &config.fadeOutEase->frameCount_, 1);

            // scaleEase interval
            ImGui::DragInt("Scale FrameCount", &config.scaleEase->frameCount_, 1);

            Vector4 startCol = Uint32ToColorVector(config.startColor);
            if (ImGui::ColorEdit4("Start Color", (float*)&startCol))
            {
                config.startColor = ColorVectorToUint32(startCol);
            }

            Vector4 endCol = Uint32ToColorVector(config.endColor);
            if (ImGui::ColorEdit4("End Color", (float*)&endCol))
            {
                config.endColor = ColorVectorToUint32(endCol);
            }
        }

        if (ImGui::CollapsingHeader("State (Live Particles)"))
        {
            int i = 0;
            for (auto& particle : particles_)
            {
                if (particle.type != type) continue;

                std::string label = "Particle[" + std::to_string(i++) + "]";
                if (ImGui::TreeNode(label.c_str()))
                {
                    ImGui::DragFloat3("Position", &particle.transform->translation_.x, 0.1f);
                    ImGui::DragFloat("Speed", &particle.speed, 0.01f);
                    ImGui::ColorEdit4("Color", &particle.color.x);
                    ImGui::Checkbox("IsExist", &particle.isExist);

                    if (ImGui::Button("Apply Config Values"))
                    {
                        particle.speed = config.speed;
                        particle.color = config.baseColor;
                    }

                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::Button("Save Configs"))
        {
            SaveConfigsToJson("Game/Data/particles.json");
        }

        ImGui::SameLine();

        if (ImGui::Button("Emit Test Particle"))
        {
            WorldTransform dummyTransform{};
            dummyTransform.Initialize();
            SpawnParticle(dummyTransform, type, config.maxLifetime, 1);
        }
    }
    ImGui::End();
}
void ParticleSystem::SaveConfigsToJson(const std::string& filepath) {
    nlohmann::json j;
    for (size_t i = 0; i < static_cast<size_t>(ParticleType::Count); ++i) {
        const auto& config = particleConfigs_[i];
        std::string typeName = ParticleTypeToString(config.type);

        nlohmann::json typeJson = {
            { "speed", config.speed },
            { "gravity", config.gravity },
            { "drag", config.drag },
            { "decayRate", config.decayRate },
            { "maxLifetime", config.maxLifetime },
            { "textureIndex", config.textureIndex },
            { "particleRadius", config.radius },
            { "baseColor", {
                config.baseColor.x,
                config.baseColor.y,
                config.baseColor.z,
                config.baseColor.w
            }},
            { "emitterRange", {
                config.emitterRange.x,
                config.emitterRange.y,
                config.emitterRange.z
            }},
            { "fadeOutFrameCount", config.fadeOutEase->frameCount_ },
            { "scaleFrameCount", config.scaleEase->frameCount_ },
            { "startColor", config.startColor },
            { "endColor", config.endColor }
        };



        j[typeName] = typeJson;
    }

    std::ofstream ofs(filepath);
    ofs << j.dump(4);
}