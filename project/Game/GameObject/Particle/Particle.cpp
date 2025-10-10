#define _USE_MATH_DEFINES

#include "Particle.h"
#include "Engine.h"
#include "MathUtils.h"
#include "TimeManager.h"
#include "ParticleEmitter.h"
#include "TextureHandle.h"
#include "KeyParticleBehavior.h"
#include "HitEffectParticleBehavior.h"
#include "ImGuiManager.h"
#include "json.hpp"

namespace
{
    static const std::vector<std::pair<const char*, TextureID>> particleTextureList =
    {
        { "white1x1", TextureID::white1x1 },
        { "circle_01", TextureID::circle_01 },
        { "circle_02", TextureID::circle_02 },
        { "circle_03", TextureID::circle_03 },
        { "circle_04", TextureID::circle_04 },
        { "circle_05", TextureID::circle_05 },
        { "dirt_01", TextureID::dirt_01 },
        { "dirt_02", TextureID::dirt_02 },
        { "dirt_03", TextureID::dirt_03 },
        { "fire_01", TextureID::fire_01 },
        { "fire_02", TextureID::fire_02 },
        { "flame_01", TextureID::flame_01 },
        { "flame_02", TextureID::flame_02 },
        { "flame_03", TextureID::flame_03 },
        { "flame_04", TextureID::flame_04 },
        { "flame_05", TextureID::flame_05 },
        { "flame_06", TextureID::flame_06 },
        { "flare_01", TextureID::flare_01 },
        { "light_01", TextureID::light_01 },
        { "light_02", TextureID::light_02 },
        { "light_03", TextureID::light_03 },
        { "magic_01", TextureID::magic_01 },
        { "magic_02", TextureID::magic_02 },
        { "magic_03", TextureID::magic_03 },
        { "magic_04", TextureID::magic_04 },
        { "magic_05", TextureID::magic_05 },
        { "muzzle_01", TextureID::muzzle_01 },
        { "muzzle_02", TextureID::muzzle_02 },
        { "muzzle_03", TextureID::muzzle_03 },
        { "muzzle_04", TextureID::muzzle_04 },
        { "muzzle_05", TextureID::muzzle_05 },
        { "scorch_01", TextureID::scorch_01 },
        { "scorch_02", TextureID::scorch_02 },
        { "scorch_03", TextureID::scorch_03 },
        { "scratch_01", TextureID::scratch_01 },
        { "slash_01", TextureID::slash_01 },
        { "slash_02", TextureID::slash_02 },
        { "slash_03", TextureID::slash_03 },
        { "slash_04", TextureID::slash_04 },
        { "smoke_01", TextureID::smoke_01 },
        { "smoke_02", TextureID::smoke_02 },
        { "smoke_03", TextureID::smoke_03 },
        { "smoke_04", TextureID::smoke_04 },
        { "smoke_05", TextureID::smoke_05 },
        { "smoke_06", TextureID::smoke_06 },
        { "smoke_07", TextureID::smoke_07 },
        { "smoke_08", TextureID::smoke_08 },
        { "smoke_09", TextureID::smoke_09 },
        { "smoke_10", TextureID::smoke_10 },
        { "spark_01", TextureID::spark_01 },
        { "spark_02", TextureID::spark_02 },
        { "spark_03", TextureID::spark_03 },
        { "spark_04", TextureID::spark_04 },
        { "spark_05", TextureID::spark_05 },
        { "spark_06", TextureID::spark_06 },
        { "spark_07", TextureID::spark_07 },
        { "star_01", TextureID::star_01 },
        { "star_02", TextureID::star_02 },
        { "star_03", TextureID::star_03 },
        { "star_04", TextureID::star_04 },
        { "star_05", TextureID::star_05 },
        { "star_06", TextureID::star_06 },
        { "star_07", TextureID::star_07 },
        { "star_08", TextureID::star_08 },
        { "star_09", TextureID::star_09 }
    };
}

ParticleSystem::ParticleSystem(){}

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

        // --- ParticleConfigの読み込み ---
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

        // --- EmitterConfigの読み込み ---
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
void ParticleSystem::ShowEditor()
{
    if (ImGui::Begin("パーティクルエディター"))
    {
        // 1. ParticleTypeを選択
        static int selectedTypeIdx = static_cast<int>(ParticleType::Key);
        ImGui::Combo("Particle Type", &selectedTypeIdx, "None\0Key\0HitEffect\0");
        ParticleType type = static_cast<ParticleType>(selectedTypeIdx);

        // 選択されたタイプのプリセット名リストを動的に作成
        std::vector<const char*> presetNames;
        std::vector<std::string> presetNameStrings; // ImGui::Comboがchar*を要求するための一時的な保持用
        if (definitions_.count(type)) {
            for (const auto& [name, def] : definitions_.at(type)) {
                presetNameStrings.push_back(name);
            }
            for (const auto& name : presetNameStrings) {
                presetNames.push_back(name.c_str());
            }
        }
        // 2. Presetを選択
        static int selectedPresetIdx = 0;
        if (presetNames.empty()) {
            ImGui::Text("No presets available for this type.");
        }
        else
        {
            if (selectedPresetIdx >= presetNames.size()) 
            { // 範囲外アクセス防止
                selectedPresetIdx = 0;
            }
            ImGui::Combo("Preset", &selectedPresetIdx, presetNames.data(), (int)presetNames.size());

            const std::string& selectedPresetName = presetNames[selectedPresetIdx];
            auto& definition = definitions_[type][selectedPresetName];
            auto& config = definition.particleConfig;
            auto& emitterConfig = definition.emitterConfig;

            if (ImGui::CollapsingHeader("Emitter Config"))
            {
                // 値が変更されたかを検出するためのフラグ
                bool valueChanged = false;

                // ImGuiの各ウィジェットが値を変更したら、valueChangedフラグを立てる
                valueChanged |= ImGui::DragFloat3("Position", &emitterConfig.position.x, 0.1f);
                valueChanged |= ImGui::DragFloat("Spawn Interval", &emitterConfig.spawnInterval, 0.01f, 0.01f, 10.0f);
                valueChanged |= ImGui::DragFloat("Particle Lifetime", &emitterConfig.lifetime, 0.01f, 0.0f, 10.0f);
                valueChanged |= ImGui::DragInt("Amount", &emitterConfig.amount, 1, 1, 100);

                // もし値が一つでも変更されていたら、ライブエミッターに設定を適用する
                if (valueChanged) {
                    ApplyEmitterConfigToLiveEmitters(type, selectedPresetName);
                }
            }

            if (ImGui::CollapsingHeader("Particle Config"))
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
                ImGui::DragInt("FadeOut FrameCount", &config.fadeOutEase.frameCount_, 1);

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

                // scaleEase interval
                ImGui::DragInt("Scale FrameCount", &config.scaleEase.frameCount_, 1);

                // scale
                ImGui::DragFloat3("Start Scale", &config.startScale.x, 0.01f);
                ImGui::DragFloat3("End Scale", &config.endScale.x, 0.01f);

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

            if (ImGui::Button("Save"))
            {
                SaveConfigToJson(type);

                std::string message = std::format("{}Particles.json saved", ParticleTypeToString(type));
                MessageBoxA(nullptr, message.c_str(), "Particles", 0);
            }

            ImGui::SameLine();

            if (ImGui::Button("Emit Test Particle"))
            {
                WorldTransform dummyTransform{};
                dummyTransform.Initialize();
                SpawnParticle(dummyTransform, type, selectedPresetName, config.maxLifetime, 1);
            }
        }
    }
    ImGui::End();
}
void ParticleSystem::SaveConfigToJson(ParticleType type) 
{
    // 指定されたタイプのプリセットマップを取得します。
 // .at(type)は、もしtypeが存在しない場合に例外を投げるので安全です。
    const auto& presets = definitions_.at(type);
    std::string typeName = ParticleTypeToString(type);

    nlohmann::json rootJson;
    nlohmann::json typeJson;


    // そのタイプの全プリセットをループしてJSONオブジェクトを構築します。
    for (const auto& [presetName, definition] : presets)
    {
        const auto& config = definition.particleConfig;
        const auto& emitterConfig = definition.emitterConfig;

        // ParticleConfigをJSONに変換 (この部分は元のコードと同じ)
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

        // EmitterConfigをJSONに変換 (この部分も元のコードと同じ)
        nlohmann::json emitterConfigJson = {
            { "position", { emitterConfig.position.x, emitterConfig.position.y, emitterConfig.position.z }},
            { "spawnInterval", emitterConfig.spawnInterval },
            { "lifetime", emitterConfig.lifetime },
            { "amount", emitterConfig.amount }
        };

        // プリセット名 (e.g., "Normal") をキーとしてJSONを構築します。
        typeJson[presetName] = {
            { "ParticleConfig", particleConfigJson },
            { "EmitterConfig", emitterConfigJson }
        };
    }

    // 最終的なJSONオブジェクトを構築します。 (e.g., { "Key": { ... } })
    rootJson[typeName] = typeJson;

    std::string filename = kConfigDirectoryPath_ + typeName + "Particles.json";
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // dump(4) でインデントを付けて見やすく出力します。
    ofs << rootJson.dump(4);
}

void ParticleSystem::ApplyEmitterConfigToLiveEmitters(ParticleType type, const std::string& presetName)
{
    // 更新する設定（設計図）を type と presetName の両方で特定します。
    const auto& emitterConfig = definitions_.at(type).at(presetName).emitterConfig;

    // 全てのライブエミッターをループ
    for (auto& emitter : emitters_) {
        // ★変更点：タイプとプリセット名の両方が一致するエミッターを見つけます。
        // ※ParticleEmitterクラスにpresetName_のようなメンバー変数を追加していることが前提です。
        if (emitter->type_ == type && emitter->presetName_ == presetName) {
            // インスタンスの値を設計図の値で上書きする
            emitter->position_ = emitterConfig.position;
            emitter->spawnInterval_ = emitterConfig.spawnInterval;
            emitter->lifetime_ = emitterConfig.lifetime;
            emitter->amount_ = emitterConfig.amount;
        }
    }
}