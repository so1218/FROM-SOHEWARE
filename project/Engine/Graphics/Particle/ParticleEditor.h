#pragma once

namespace FE
{

class Engine;
class ParticleSystem;

class ParticleEditor
{
public:
    ParticleEditor(ParticleSystem* system, Engine* engine);

    void ShowEditor();

    void ApplyEmitterConfigToLiveEmitters(const std::string& presetName);

    void ResetSelection() {
        selectedPresetIdx_ = 0;
        selectedTextureIdx_ = 0;
    }

private:
    Engine* engine_;
    ParticleSystem* particleSystem_;

    int selectedPresetIdx_ = 0;
    int selectedTextureIdx_ = 0;

    // 保存メッセージの表示残り時間
    float saveMessageTimer_ = 0.0f;
};

}

