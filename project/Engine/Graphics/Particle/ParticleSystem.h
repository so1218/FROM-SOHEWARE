#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "WorldTransform.h"
#include "Easing.h"
#include "Structures.h"
#include "ParticleDefinition.h"

#include <wrl.h>
#include <d3d12.h>
#include <vector>
#include <array>
#include <fstream>
#include <map>

class Engine;
class ParticleEmitter;
class ParticleEditor;
class ParticleConfigManager;

class ParticleSystem
{
public:
    const char* kConfigDirectoryPath_ = "Assets/Data/Particle/";

    ParticleSystem(Engine* engine);
    ~ParticleSystem();

    void Initialize();
    void SpawnParticle(const WorldTransform& transform, const std::string& presetName, float lifetime,
        const WorldTransform* attractionTarget, const WorldTransform* vortexTarget, const ModelData* emitterModelData = nullptr);
    void Update();
    void AddEmitter(std::unique_ptr<ParticleEmitter> emitter);
    std::unique_ptr<ParticleEmitter> CreateEmitter(const std::string& presetName);
    void Draw();
    // presetNameだけでConfigを取得できるように
    const ParticleConfig& GetConfig(const std::string& presetName) const { return definitions_.at(presetName).particleConfig; }
    ParticleConfig& GetConfig(const std::string& presetName) { return definitions_.at(presetName).particleConfig; }
    void Clear();

public:
    bool ShouldSkipDraw(const ParticleState& particle) const;

    Engine* engine_;
    std::vector<std::unique_ptr<ParticleEmitter>> emitters_;
    std::unique_ptr<ParticleEditor> editor_;
    std::unique_ptr<ParticleConfigManager> configManager_;
    std::vector<ParticleState> particles_;
    // プリセット名(string)をキーとして、定義(ParticleDefinition)をマッピング
    std::map<std::string, ParticleDefinition> definitions_;
    std::unordered_map<std::string, ParticleEmitter*> namedEmitters_;
};


