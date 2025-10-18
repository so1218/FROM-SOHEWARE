#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "WorldTransform.h"
#include "Easing.h"
#include "Structures.h"
#include "IParticleBehavior.h"

#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <array>
#include <fstream>
#include <map>

class Engine;
class ParticleEmitter;
class ParticleEditor;

class ParticleSystem
{
public:
    static constexpr const char* kConfigDirectoryPath_ = "Resources/json/";

    ParticleSystem();
    ~ParticleSystem();

    void Initialize(Engine* engine);
    void SpawnParticle(WorldTransform& transform, ParticleType type, const std::string& presetName, float lifetime, int amount);
    void Update();
    void AddEmitter(std::unique_ptr<ParticleEmitter> emitter);
    void LoadParticleDefinitionFromJson(ParticleType type);
    std::unique_ptr<ParticleEmitter> CreateEmitter(ParticleType type, const std::string& presetName);
    void Draw();
    void SaveConfigToJson(ParticleType type);
    void ApplyEmitterConfigToLiveEmitters(ParticleType type, const std::string& presetName);
    const ParticleConfig& GetConfig(ParticleType type, const std::string& presetName) const { return definitions_.at(type).at(presetName).particleConfig; }
    ParticleConfig& GetConfig(ParticleType type, const std::string& presetName) { return definitions_.at(type).at(presetName).particleConfig; }

public:
    Engine* engine_;
    std::vector<std::unique_ptr<ParticleEmitter>> emitters_;  // エミッターのリスト
    std::unique_ptr<ParticleEditor> editor_;
    std::vector<ParticleState> particles_;
    std::map<ParticleType, std::map<std::string, ParticleDefinition>> definitions_;
    std::unordered_map<ParticleType, std::unique_ptr<IParticleBehavior>> behaviors_;
};


