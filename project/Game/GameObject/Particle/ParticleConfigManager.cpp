#include "ParticleConfigManager.h" 
#include "json.hpp"    

#include <iostream>

ParticleConfigManager::ParticleConfigManager(ParticleSystem* particleSystem)
    : particleSystem_(particleSystem)
{
}

void ParticleConfigManager::LoadAllParticleDefinitions()
{
    const std::string& directoryPath = particleSystem_->kConfigDirectoryPath_;

    // ディレクトリが存在しない場合は何もしない
    if (!std::filesystem::exists(directoryPath)) {
        std::cerr << "Particle config directory not found: " << directoryPath << std::endl;
        return;
    }

    // ディレクトリ内の全ファイルを走査
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        // .json ファイルのみを対象とする
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            std::string presetName = entry.path().stem().string(); // ファイル名(拡張子なし)をプリセット名とする
            std::ifstream file(entry.path());

            if (!file.is_open()) continue;

            nlohmann::json j;
            file >> j;

            // 新しいマップに直接定義を読み込む
            auto& definition = particleSystem_->definitions_[presetName];

            // ParticleConfigの読み込み
            if (j.contains("ParticleConfig"))
            {
                auto& configJson = j["ParticleConfig"];
                auto& config = definition.particleConfig; // ParticleConfigへの参照を取得

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

                // ColorOverLifetimeModuleの読み込み
                if (configJson.contains("colorOverLifetimeModule"))
                {
                    auto& colorJson = configJson["colorOverLifetimeModule"];
                    config.colorOverLifetime.enabled = colorJson.value("enabled", false);
                    config.colorOverLifetime.startColor = colorJson.value("startColor", 0xFFFFFFFF);
                    config.colorOverLifetime.endColor = colorJson.value("endColor", 0xFFFFFF00);
                    /*  EasingType easingType = static_cast<EasingType>(colorJson.value("easingType", static_cast<int>(EasingType::EaseLinear)));
                      config.colorOverLifetime.easing.SetEasing(easingType);*/
                }

                // SizeOverLifetimeModuleの読み込み
                if (configJson.contains("sizeOverLifetimeModule"))
                {
                    auto& sizeJson = configJson["sizeOverLifetimeModule"];
                    config.sizeOverLifetime.enabled = sizeJson.value("enabled", false);
                    if (sizeJson.contains("startScale")) {
                        config.sizeOverLifetime.startScale = { sizeJson["startScale"][0], sizeJson["startScale"][1], sizeJson["startScale"][2] };
                    }
                    if (sizeJson.contains("endScale")) {
                        config.sizeOverLifetime.endScale = { sizeJson["endScale"][0], sizeJson["endScale"][1], sizeJson["endScale"][2] };
                    }
                    /*  EasingType easingType = static_cast<EasingType>(sizeJson.value("easingType", static_cast<int>(EasingType::EaseLinear)));
                      config.sizeOverLifetime.easing.SetEasing(easingType);*/
                    config.sizeOverLifetime.oscillate = sizeJson.value("oscillate", false);
                    config.sizeOverLifetime.frequency = sizeJson.value("frequency", 1.0f);
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
            if (j.contains("EmitterConfig"))
            {
                auto& emitterJson = j["EmitterConfig"];
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
}

void ParticleConfigManager::SaveParticleDefinitionToJson(const std::string& presetName)
{
    // 指定されたタイプのプリセットマップを取得
   // 指定されたプリセットの定義を取得
    const auto& definition = particleSystem_->definitions_.at(presetName);
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
        // ColorOverLifetimeModuleの情報
        { "colorOverLifetimeModule",
        {
            { "enabled", config.colorOverLifetime.enabled },
            { "startColor", config.colorOverLifetime.startColor },
            { "endColor", config.colorOverLifetime.endColor },
            /*{ "easingType", static_cast<int>(config.colorOverLifetime.easing.GetEasingType()) }*/
        }},

        // SizeOverLifetimeModuleの情報
        { "sizeOverLifetimeModule", {
            { "enabled", config.sizeOverLifetime.enabled },
            { "startScale", { config.sizeOverLifetime.startScale.x, config.sizeOverLifetime.startScale.y, config.sizeOverLifetime.startScale.z }},
            { "endScale", { config.sizeOverLifetime.endScale.x, config.sizeOverLifetime.endScale.y, config.sizeOverLifetime.endScale.z }},
            /*{ "easingType", static_cast<int>(config.sizeOverLifetime.easing.GetEasingType()) },*/
            { "oscillate", config.sizeOverLifetime.oscillate },
            { "frequency", config.sizeOverLifetime.frequency }
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
    nlohmann::json rootJson =
    {
        { "ParticleConfig", particleConfigJson },
        { "EmitterConfig", emitterConfigJson }
    };

    // ファイル名をプリセット名から生成
    std::string filename = particleSystem_->kConfigDirectoryPath_ + presetName + ".json";
    std::ofstream ofs(filename);
    if (!ofs)
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    ofs << rootJson.dump(4); // 見やすくインデント付きで出力
}