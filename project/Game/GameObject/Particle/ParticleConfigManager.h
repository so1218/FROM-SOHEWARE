#pragma once

#pragma once
#include "ParticleSystem.h" 

class ParticleConfigManager
{
public:
    ParticleConfigManager(ParticleSystem* system);

    // ディレクトリ内の全パーティクル定義を読み込む関数
    void LoadAllParticleDefinitions();

    // 保存処理：同様に、保存したいdefinitions_を引数で受け取る
    void SaveParticleDefinitionToJson(const std::string& presetName);

private:

    ParticleSystem* particleSystem_;
};