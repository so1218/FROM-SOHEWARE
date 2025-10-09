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

class ParticleSystem
{
public:
    static constexpr const char* kConfigDirectoryPath_ = "Resources/json/";

    void Initialize(Engine* engine);
    void SpawnParticle(WorldTransform& transform, ParticleType type, float lifetime, int amount);
    void Update();
    void AddEmitter(ParticleEmitter* emitter);
    void LoadParticleDefinitionFromJson(ParticleType type);
    std::unique_ptr<ParticleEmitter> CreateEmitter(ParticleType type);
    void ShowEditor();
    void SaveConfigToJson(ParticleType type);
    void ApplyEmitterConfigToLiveEmitters(ParticleType type);
    const ParticleConfig& GetConfig(ParticleType type) const{ return definitions_.at(type).particleConfig; }
    ParticleConfig& GetConfig(ParticleType type) { return definitions_.at(type).particleConfig; }

public:
    Engine* engine_;
    std::vector<ParticleEmitter*> emitters_;  // エミッターのリスト
    std::vector<ParticleState> particles_;
    std::map<ParticleType, ParticleDefinition> definitions_;
    std::unordered_map<ParticleType, std::unique_ptr<IParticleBehavior>> behaviors_;
};


