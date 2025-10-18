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
            // VelocityModuleの読み込み
            if (configJson.contains("velocityModule"))
            {
                auto& velJson = configJson["velocityModule"];
                config.velocity.enabled = velJson.value("enabled", false);
                config.velocity.speed = velJson.value("speed", 1.0f);
                config.velocity.randomDirection = velJson.value("randomDirection", false);
                config.velocity.angleRange = velJson.value("angleRange", 90.0f);
                if (velJson.contains("direction") && velJson["direction"].is_array() && velJson["direction"].size() == 3)
                {
                    config.velocity.direction = {
                        velJson["direction"][0].get<float>(),
                        velJson["direction"][1].get<float>(),
                        velJson["direction"][2].get<float>()
                    };
                }
            }
            // PhysicsModuleの読み込み
            if (configJson.contains("physicsModule"))
            {
                auto& physJson = configJson["physicsModule"];
                config.physics.enabled = physJson.value("enabled", false);
                config.physics.gravity = physJson.value("gravity", 0.0f);
                config.physics.drag = physJson.value("drag", 0.0f);
            }

            // RotationOverLifetimeModuleの読み込みを追加
            if (configJson.contains("rotationModule"))
            {
                auto& rotJson = configJson["rotationModule"];
                config.rotation.enabled = rotJson.value("enabled", false);
                config.rotation.randomStartRotation = rotJson.value("randomStartRotation", true);
                config.rotation.angularVelocity = rotJson.value("angularVelocity", 5.0f);
            }

            // ShapeModuleの読み込み
            if (configJson.contains("shapeModule"))
            {
                auto& shapeJson = configJson["shapeModule"];
                config.shape.enabled = shapeJson.value("enabled", true);
                config.shape.type = static_cast<ShapeModule::Type>(shapeJson.value("type", static_cast<int>(ShapeModule::Type::Circle)));
                config.shape.radius = shapeJson.value("radius", 10.0f);
                config.shape.emitFromEdge = shapeJson.value("emitFromEdge", false);
                if (shapeJson.contains("boxSize") && shapeJson["boxSize"].is_array())
                {
                    config.shape.boxSize = {
                        shapeJson["boxSize"][0].get<float>(),
                        shapeJson["boxSize"][1].get<float>(),
                        shapeJson["boxSize"][2].get<float>()
                    };
                }
            }

            // TextureSheetAnimationModuleの読み込み
            if (configJson.contains("textureSheetModule"))
            {
                auto& texJson = configJson["textureSheetModule"];
                config.textureSheet.enabled = texJson.value("enabled", false);
                config.textureSheet.textureHandle = texJson.value("textureHandle", 0);
                config.textureSheet.tilesX = texJson.value("tilesX", 1);
                config.textureSheet.tilesY = texJson.value("tilesY", 1);
                config.textureSheet.framesPerSecond = texJson.value("framesPerSecond", 10.0f);
                config.textureSheet.looping = texJson.value("looping", true);
            }

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
            { "velocityModule",
            {
                { "enabled", config.velocity.enabled },
                { "speed", config.velocity.speed },
                { "randomDirection", config.velocity.randomDirection },
                { "angleRange", config.velocity.angleRange },
                { "direction", { config.velocity.direction.x, config.velocity.direction.y, config.velocity.direction.z }}
            }},
            // PhysicsModule
            { "physicsModule", {
                { "enabled", config.physics.enabled },
                { "gravity", config.physics.gravity },
                { "drag", config.physics.drag }
            }},

            // RotationOverLifetimeModule
            { "rotationModule", {
                { "enabled", config.rotation.enabled },
                { "randomStartRotation", config.rotation.randomStartRotation },
                { "angularVelocity", config.rotation.angularVelocity }
            }},
            { "shapeModule",
            {
                { "enabled", config.shape.enabled },
                { "type", static_cast<int>(config.shape.type) }, // Enumは整数として保存
                { "radius", config.shape.radius },
                { "emitFromEdge", config.shape.emitFromEdge },
                { "boxSize", { config.shape.boxSize.x, config.shape.boxSize.y, config.shape.boxSize.z }}
            }},
            { "textureSheetModule", 
            {
                { "enabled", config.textureSheet.enabled },
                { "textureHandle", config.textureSheet.textureHandle },
                { "tilesX", config.textureSheet.tilesX },
                { "tilesY", config.textureSheet.tilesY },
                { "framesPerSecond", config.textureSheet.framesPerSecond },
                { "looping", config.textureSheet.looping }
            }},
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