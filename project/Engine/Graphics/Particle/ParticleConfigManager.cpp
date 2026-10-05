#include "pch.h"
#include "ParticleConfigManager.h" 
#include "ParticleSystem.h" 

namespace FE
{

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
            std::string presetName = entry.path().stem().string(); // ファイル名(拡張子なし)がプリセット名
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
                auto& config = definition.particleConfig; 

                config.intensity = configJson.value("intensity", 1.0f);

                config.blendMode = static_cast<BlendMode>(configJson.value("blendMode", static_cast<int>(BlendMode::kBlendModeAdd)));

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
                    if (physJson.contains("gravity") && physJson["gravity"].is_array())
                    {
                        config.physics.gravity =
                        {
                            physJson["gravity"][0].get<float>(),
                            physJson["gravity"][1].get<float>(),
                            physJson["gravity"][2].get<float>()
                        };
                    }
                    config.physics.drag = physJson.value("drag", 0.0f);
                    config.physics.affectedByWind = physJson.value("affectedByWind", false);
                    config.physics.windInfluence = physJson.value("windInfluence", 1.0f);
                }

                // RotationOverLifetimeModuleの読み込み
                if (configJson.contains("rotationModule"))
                {
                    auto& rotJson = configJson["rotationModule"];
                    config.rotation.enabled = rotJson.value("enabled", false);
                    config.rotation.isBillboard = rotJson.value("isBillboard", true);

                    config.rotation.minAngularVelocity2D = rotJson.value("minAngularVelocity2D", 0.0f);
                    config.rotation.maxAngularVelocity2D = rotJson.value("maxAngularVelocity2D", 0.0f);

                    if (rotJson.contains("minAngularVelocity3D")) {
                        auto& v = rotJson["minAngularVelocity3D"];
                        config.rotation.minAngularVelocity3D = { v[0], v[1], v[2] };
                    }
                    if (rotJson.contains("maxAngularVelocity3D")) {
                        auto& v = rotJson["maxAngularVelocity3D"];
                        config.rotation.maxAngularVelocity3D = { v[0], v[1], v[2] };
                    }

                    if (rotJson.contains("minStartRotation")) {
                        auto& v = rotJson["minStartRotation"];
                        config.rotation.minStartRotation = { v[0], v[1], v[2] };
                    }
                    if (rotJson.contains("maxStartRotation")) {
                        auto& v = rotJson["maxStartRotation"];
                        config.rotation.maxStartRotation = { v[0], v[1], v[2] };
                    }
                }

                // ShapeModuleの読み込み
                if (configJson.contains("shapeModule"))
                {
                    auto& shapeJson = configJson["shapeModule"];
                    config.shape.enabled = shapeJson.value("enabled", true);
                    config.shape.type = static_cast<ShapeModule::Type>(shapeJson.value("type", static_cast<int>(ShapeModule::Type::Point)));
                    if (shapeJson.contains("radius") && shapeJson["radius"].is_array())
                    {
                        config.shape.radius = {
                            shapeJson["radius"][0].get<float>(),
                            shapeJson["radius"][1].get<float>(),
                            shapeJson["radius"][2].get<float>()
                        };
                    }
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
                    config.textureSheet.textureName = texJson.value("textureName", "white1x1");
                }

                // ColorOverLifetimeModuleの読み込み
                if (configJson.contains("colorOverLifetimeModule"))
                {
                    auto& colorJson = configJson["colorOverLifetimeModule"];
                    config.colorOverLifetime.enabled = colorJson.value("enabled", false);
                    config.colorOverLifetime.mode = static_cast<ColorOverLifetimeModule::Mode>(
                        colorJson.value("mode", static_cast<int>(ColorOverLifetimeModule::Mode::Single))
                        );
                    // グラデーション1
                    config.colorOverLifetime.startColor = colorJson.value("startColor", 0xFFFFFFFF);
                    config.colorOverLifetime.endColor = colorJson.value("endColor", 0xFFFFFF00);

                    // グラデーション2の読み込み 
                    config.colorOverLifetime.startColor2 = colorJson.value("startColor2", config.colorOverLifetime.startColor);
                    config.colorOverLifetime.endColor2 = colorJson.value("endColor2", config.colorOverLifetime.endColor);
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
                    config.sizeOverLifetime.oscillate = sizeJson.value("oscillate", false);
                    config.sizeOverLifetime.frequency = sizeJson.value("frequency", 1.0f);
                }
                // VortexModuleの読み込み
                if (configJson.contains("vortexModule"))
                {
                    auto& vortexJson = configJson["vortexModule"];
                    config.vortex.enabled = vortexJson.value("enabled", false);

                    if (vortexJson.contains("center")) {
                        config.vortex.center = { vortexJson["center"][0], vortexJson["center"][1], vortexJson["center"][2] };
                    }
                    if (vortexJson.contains("offset")) {
                        config.vortex.offset = { vortexJson["offset"][0], vortexJson["offset"][1], vortexJson["offset"][2] };
                    }
                    else {
                        config.vortex.offset = { 0.0f, 0.0f, 0.0f };
                    }

                    if (vortexJson.contains("axis")) {
                        config.vortex.axis = { vortexJson["axis"][0], vortexJson["axis"][1], vortexJson["axis"][2] };
                    }
                    else {
                        config.vortex.axis = { 0.0f, 1.0f, 0.0f };
                    }

                    config.vortex.orbitalSpeed = vortexJson.value("orbitalSpeed", 2.0f);
                    config.vortex.radialSpeed = vortexJson.value("radialSpeed", 0.0f);
                }
                // AttractionModuleの読み込み
                if (configJson.contains("attractionModule"))
                {
                    auto& attrJson = configJson["attractionModule"];
                    config.attraction.enabled = attrJson.value("enabled", false);
                    if (attrJson.contains("target")) {
                        config.attraction.target = { attrJson["target"][0], attrJson["target"][1], attrJson["target"][2] };
                    }
                    if (attrJson.contains("offset") && attrJson["offset"].is_array()) {
                        config.attraction.offset = {
                            attrJson["offset"][0], attrJson["offset"][1], attrJson["offset"][2]
                        };
                    }
                    config.attraction.strength = attrJson.value("strength", 1.0f);
                }
                if (configJson.contains("trailModule"))
                {
                    auto& trailJson = configJson["trailModule"];
                    config.trail.enabled = trailJson.value("enabled", false);
                    config.trail.lifetime = trailJson.value("lifetime", 0.5f);
                    config.trail.width = trailJson.value("width", 1.0f);
                    config.trail.emissiveIntensity = trailJson.value("emissiveIntensity", 1.0f);
                    config.trail.minVertexDistance = trailJson.value("minVertexDistance", 0.1f);
                    config.trail.textureName = trailJson.value("textureName", "white1x1");
                    config.trail.jitterMode = static_cast<JitterMode>(trailJson.value("jitterMode", 0));
                    config.trail.jitterStrength = trailJson.value("jitterStrength", 0.0f);
                    config.trail.jitterFrequency = trailJson.value("jitterFrequency", 10.0f);
                    config.trail.jitterSpeed = trailJson.value("jitterSpeed", 0.0f);
                    config.trail.jitterPhase = trailJson.value("jitterPhase", 0.0f);

                    config.trail.headWidthScale = trailJson.value("headWidthScale", 1.0f);
                    config.trail.tailWidthScale = trailJson.value("tailWidthScale", 1.0f);

                    config.trail.alignment = static_cast<TrailAlignment>(trailJson.value("alignment", 0));
                    if (trailJson.contains("startColor") && trailJson["startColor"].is_array())
                    {
                        config.trail.startColor = {
                            trailJson["startColor"][0].get<float>(),
                            trailJson["startColor"][1].get<float>(),
                            trailJson["startColor"][2].get<float>(),
                            trailJson["startColor"][3].get<float>()
                        };
                    }
                    if (trailJson.contains("endColor") && trailJson["endColor"].is_array())
                    {
                        config.trail.endColor = {
                            trailJson["endColor"][0].get<float>(),
                            trailJson["endColor"][1].get<float>(),
                            trailJson["endColor"][2].get<float>(),
                            trailJson["endColor"][3].get<float>()
                        };
                    }
                }
                if (configJson.contains("noiseModule"))
                {
                    auto& noiseJson = configJson["noiseModule"];
                    config.noise.enabled = noiseJson.value("enabled", false);
                    config.noise.type = static_cast<NoiseModule::NoiseType>(noiseJson.value("type", 0));
                    config.noise.strength = noiseJson.value("strength", 1.0f);
                    config.noise.frequency = noiseJson.value("frequency", 0.05f);
                    config.noise.scrollSpeed = noiseJson.value("scrollSpeed", 0.5f);
                    config.noise.separateAxes = noiseJson.value("separateAxes", true);
                }
            }

            // EmitterConfigの読み込み
            if (j.contains("EmitterConfig"))
            {
                auto& emitterJson = j["EmitterConfig"];
                auto& emitterConfig = definition.emitterConfig; 

                if (emitterJson.contains("position") && emitterJson["position"].is_array())
                {
                    emitterConfig.position =
                    {
                        emitterJson["position"][0].get<float>(),
                        emitterJson["position"][1].get<float>(),
                        emitterJson["position"][2].get<float>()
                    };
                }
                if (emitterJson.contains("followOffset") && emitterJson["followOffset"].is_array())
                {
                    emitterConfig.followOffset =
                    {
                        emitterJson["followOffset"][0].get<float>(),
                        emitterJson["followOffset"][1].get<float>(),
                        emitterJson["followOffset"][2].get<float>()
                    };
                }
                emitterConfig.spawnInterval = emitterJson.value("spawnInterval", 0.1f);
                emitterConfig.lifetime = emitterJson.value("lifetime", 5.0f);
                emitterConfig.amount = emitterJson.value("amount", 1);
                emitterConfig.duration = emitterJson.value("duration", 5.0f);
                emitterConfig.looping = emitterJson.value("looping", true);
                emitterConfig.playOnAwake = emitterJson.value("playOnAwake", true);
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
        { "intensity", config.intensity },
        { "blendMode", static_cast<int>(config.blendMode) },

        { "velocityModule",
        {
            { "enabled", config.velocity.enabled },
            { "speed", config.velocity.speed },
            { "randomDirection", config.velocity.randomDirection },
            { "angleRange", config.velocity.angleRange },
            { "direction", { config.velocity.direction.x, config.velocity.direction.y, config.velocity.direction.z }}
        }},
        { "physicsModule",
        {
            { "enabled", config.physics.enabled },
            { "gravity", { config.physics.gravity.x, config.physics.gravity.y, config.physics.gravity.z }},
            { "drag", config.physics.drag },
            { "affectedByWind", config.physics.affectedByWind },
            { "windInfluence", config.physics.windInfluence }
        }},
        { "rotationModule",
        {
            { "enabled", config.rotation.enabled },
            { "isBillboard", config.rotation.isBillboard },

            { "minAngularVelocity2D", config.rotation.minAngularVelocity2D },
            { "maxAngularVelocity2D", config.rotation.maxAngularVelocity2D },

            { "minAngularVelocity3D", { config.rotation.minAngularVelocity3D.x, config.rotation.minAngularVelocity3D.y, config.rotation.minAngularVelocity3D.z } },
            { "maxAngularVelocity3D", { config.rotation.maxAngularVelocity3D.x, config.rotation.maxAngularVelocity3D.y, config.rotation.maxAngularVelocity3D.z } },

            { "minStartRotation", { config.rotation.minStartRotation.x, config.rotation.minStartRotation.y, config.rotation.minStartRotation.z } },
            { "maxStartRotation", { config.rotation.maxStartRotation.x, config.rotation.maxStartRotation.y, config.rotation.maxStartRotation.z } },
        }},
        { "shapeModule",
        {
            { "enabled", config.shape.enabled },
            { "type", static_cast<int>(config.shape.type) },
            { "radius", { config.shape.radius.x, config.shape.radius.y, config.shape.radius.z }},
            { "emitFromEdge", config.shape.emitFromEdge },
            { "boxSize", { config.shape.boxSize.x, config.shape.boxSize.y, config.shape.boxSize.z }}
        }},
        { "textureSheetModule",
        {
            { "enabled", config.textureSheet.enabled },
            { "textureName", config.textureSheet.textureName },
        }},
        { "colorOverLifetimeModule",
        {
            { "enabled", config.colorOverLifetime.enabled },
            { "mode", static_cast<int>(config.colorOverLifetime.mode) },

            { "startColor", config.colorOverLifetime.startColor },
            { "endColor", config.colorOverLifetime.endColor },

            { "startColor2", config.colorOverLifetime.startColor2 },
            { "endColor2", config.colorOverLifetime.endColor2 },
        }},
        { "sizeOverLifetimeModule",
        {
            { "enabled", config.sizeOverLifetime.enabled },
            { "startScale", { config.sizeOverLifetime.startScale.x, config.sizeOverLifetime.startScale.y, config.sizeOverLifetime.startScale.z }},
            { "endScale", { config.sizeOverLifetime.endScale.x, config.sizeOverLifetime.endScale.y, config.sizeOverLifetime.endScale.z }},
            { "oscillate", config.sizeOverLifetime.oscillate },
            { "frequency", config.sizeOverLifetime.frequency }
        }},
        { "vortexModule",
        {
            { "enabled", config.vortex.enabled },
            { "center", { config.vortex.center.x, config.vortex.center.y, config.vortex.center.z }},
            { "offset", { config.vortex.offset.x, config.vortex.offset.y, config.vortex.offset.z }},
            { "axis", { config.vortex.axis.x, config.vortex.axis.y, config.vortex.axis.z }},
            { "orbitalSpeed", config.vortex.orbitalSpeed },
            { "radialSpeed", config.vortex.radialSpeed }
        }},
        { "attractionModule",
        {
            { "enabled", config.attraction.enabled },
            { "target", { config.attraction.target.x, config.attraction.target.y, config.attraction.target.z }},
            { "offset", { config.attraction.offset.x, config.attraction.offset.y, config.attraction.offset.z } },
            { "strength", config.attraction.strength }
        }},
        { "trailModule",
        {
            { "enabled", config.trail.enabled },
            { "lifetime", config.trail.lifetime },
            { "width", config.trail.width },
            { "emissiveIntensity", config.trail.emissiveIntensity },
            { "minVertexDistance", config.trail.minVertexDistance },
            { "textureName", config.trail.textureName },
            { "jitterMode", static_cast<int>(config.trail.jitterMode) },
            { "jitterStrength", config.trail.jitterStrength },
            { "jitterFrequency", config.trail.jitterFrequency },
            { "jitterSpeed", config.trail.jitterSpeed },
            { "jitterPhase", config.trail.jitterPhase },
            { "headWidthScale", config.trail.headWidthScale },
            { "tailWidthScale", config.trail.tailWidthScale },
            { "alignment", static_cast<int>(config.trail.alignment) },

            { "startColor", {
                config.trail.startColor.x,
                config.trail.startColor.y,
                config.trail.startColor.z,
                config.trail.startColor.w
            }},
            { "endColor", {
                config.trail.endColor.x,
                config.trail.endColor.y,
                config.trail.endColor.z,
                config.trail.endColor.w
            }}
        }},
        { "noiseModule",
        {
            { "enabled", config.noise.enabled },
            { "type", static_cast<int>(config.noise.type) },
            { "strength", config.noise.strength },
            { "frequency", config.noise.frequency },
            { "scrollSpeed", config.noise.scrollSpeed },
            { "separateAxes", config.noise.separateAxes }
        }}
    };

    // EmitterConfigをJSONに変換
    nlohmann::json emitterConfigJson =
    {
        { "position", { emitterConfig.position.x, emitterConfig.position.y, emitterConfig.position.z }},
        { "followOffset", { emitterConfig.followOffset.x, emitterConfig.followOffset.y, emitterConfig.followOffset.z }},
        { "spawnInterval", emitterConfig.spawnInterval },
        { "lifetime", emitterConfig.lifetime },
        { "amount", emitterConfig.amount },
        { "duration", emitterConfig.duration },
        { "looping", emitterConfig.looping },
        { "playOnAwake", emitterConfig.playOnAwake },
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

}