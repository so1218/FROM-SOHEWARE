#include "ParticleConfigManager.h" 
#include "json.hpp"    

#include <iostream>

ParticleConfigManager::ParticleConfigManager(ParticleSystem* particleSystem)
    : particleSystem_(particleSystem)
{
}

void ParticleConfigManager::LoadParticleDefinitionFromJson(ParticleType type)
{
    std::string typeName = ParticleTypeToString(type);
    std::string filepath = particleSystem_->kConfigDirectoryPath_ + typeName + "Particles.json";

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
        auto& definition = particleSystem_->definitions_[type][presetName];

        // ParticleConfigの読み込み
        if (presetJson.contains("ParticleConfig"))
        {
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

            if (configJson.contains("baseColor") && configJson["baseColor"].is_array())
            {
                auto colorArray = configJson["baseColor"];
                config.baseColor =
                {
                    colorArray[0].get<float>(),
                    colorArray[1].get<float>(),
                    colorArray[2].get<float>(),
                    colorArray[3].get<float>()
                };
            }

            if (configJson.contains("emitterRange") && configJson["emitterRange"].is_array())
            {
                auto& r = configJson["emitterRange"];
                config.emitterRange =
                {
                    r[0].get<float>(),
                    r[1].get<float>(),
                    r[2].get<float>()
                };
            }

            config.fadeOutEase.frameCount_ = configJson.value("fadeOutFrameCount", 60);
            config.startColor = configJson.value("startColor", 0xffffffff);
            config.endColor = configJson.value("endColor", 0xffffffff);
            config.scaleEase.frameCount_ = configJson.value("scaleFrameCount", 60);
            if (configJson.contains("startScale") && configJson["startScale"].is_array())
            {
                auto& arr = configJson["startScale"];
                config.startScale =
                {
                    arr[0].get<float>(),
                    arr[1].get<float>(),
                    arr[2].get<float>()
                };
            }

            if (configJson.contains("endScale") && configJson["endScale"].is_array())
            {
                auto& arr = configJson["endScale"];
                config.endScale =
                {
                    arr[0].get<float>(),
                    arr[1].get<float>(),
                    arr[2].get<float>()
                };
            }
        }

        // EmitterConfigの読み込み
        if (presetJson.contains("EmitterConfig"))
        {
            auto& emitterJson = presetJson["EmitterConfig"];
            auto& emitterConfig = definition.emitterConfig; // EmitterConfigへの参照を取得

            if (emitterJson.contains("position") && emitterJson["position"].is_array())
            {
                emitterConfig.position =
                {
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

void ParticleConfigManager::SaveConfigToJson(ParticleType type)
{
    // 指定されたタイプのプリセットマップを取得
    const auto& presets = particleSystem_->definitions_.at(type);
    std::string typeName = ParticleTypeToString(type);

    nlohmann::json rootJson;
    nlohmann::json typeJson;

    // そのタイプの全プリセットをループしてJSONオブジェクトを構築
    for (const auto& [presetName, definition] : presets)
    {
        const auto& config = definition.particleConfig;
        const auto& emitterConfig = definition.emitterConfig;

        // ParticleConfigをJSONに変換
        nlohmann::json particleConfigJson =
        {
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
        nlohmann::json emitterConfigJson =
        {
            { "position", { emitterConfig.position.x, emitterConfig.position.y, emitterConfig.position.z }},
            { "spawnInterval", emitterConfig.spawnInterval },
            { "lifetime", emitterConfig.lifetime },
            { "amount", emitterConfig.amount }
        };

        // プリセット名をキーとしてJSONを構築
        typeJson[presetName] =
        {
            { "ParticleConfig", particleConfigJson },
            { "EmitterConfig", emitterConfigJson }
        };
    }

    // 最終的なJSONオブジェクトを構築
    rootJson[typeName] = typeJson;

    std::string filename = particleSystem_->kConfigDirectoryPath_ + typeName + "Particles.json";
    std::ofstream ofs(filename);
    if (!ofs)
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // dump(4) でインデントを付けて見やすく出力
    ofs << rootJson.dump(4);
}