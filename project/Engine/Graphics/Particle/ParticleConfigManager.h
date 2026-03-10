#pragma once
class ParticleSystem;

class ParticleConfigManager
{
public:
    ParticleConfigManager(ParticleSystem* system);

    // ディレクトリ内の全パーティクル定義を読み込む関数
    void LoadAllParticleDefinitions();

    // 保存処理
    void SaveParticleDefinitionToJson(const std::string& presetName);

private:

    ParticleSystem* particleSystem_;
};