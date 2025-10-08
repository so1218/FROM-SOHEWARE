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

class Engine;
class ParticleEmitter;

class ParticleSystem
{
public:
    void Initialize(Engine* engine);
    void SpawnParticle(WorldTransform& transform, ParticleType type, float lifetime, int amount);
    void Update();
    void AddEmitter(ParticleEmitter* emitter);
    void LoadParticleDefinitionsFromJson(const std::string& filepath);
    void ShowEditor();
    void SaveConfigsToJson(const std::string& filepath);
    const ParticleConfig& GetConfig(ParticleType type) const{ return particleConfigs_[static_cast<size_t>(type)]; }
    ParticleConfig& GetConfig(ParticleType type) { return particleConfigs_[static_cast<size_t>(type)]; }

private:

public:
    Engine* engine_;
    std::vector<ParticleEmitter*> emitters_;  // エミッターのリスト
    std::vector<ParticleState> particles_;
    std::array<ParticleConfig, static_cast<size_t>(ParticleType::Count)> particleConfigs_;
    std::unordered_map<ParticleType, std::unique_ptr<IParticleBehavior>> behaviors_;
};


