#include "ParticleEditor.h"
#include "imGuiManager.h"
#include "TextureHandle.h"

namespace
{
    static const std::vector<std::pair<const char*, TextureID>> particleTextureList =
    {
        { "white1x1", TextureID::white1x1 },
        { "circle_01", TextureID::circle_01 },
        { "circle_02", TextureID::circle_02 },
        { "circle_03", TextureID::circle_03 },
        { "circle_04", TextureID::circle_04 },
        { "circle_05", TextureID::circle_05 },
        { "dirt_01", TextureID::dirt_01 },
        { "dirt_02", TextureID::dirt_02 },
        { "dirt_03", TextureID::dirt_03 },
        { "fire_01", TextureID::fire_01 },
        { "fire_02", TextureID::fire_02 },
        { "flame_01", TextureID::flame_01 },
        { "flame_02", TextureID::flame_02 },
        { "flame_03", TextureID::flame_03 },
        { "flame_04", TextureID::flame_04 },
        { "flame_05", TextureID::flame_05 },
        { "flame_06", TextureID::flame_06 },
        { "flare_01", TextureID::flare_01 },
        { "light_01", TextureID::light_01 },
        { "light_02", TextureID::light_02 },
        { "light_03", TextureID::light_03 },
        { "magic_01", TextureID::magic_01 },
        { "magic_02", TextureID::magic_02 },
        { "magic_03", TextureID::magic_03 },
        { "magic_04", TextureID::magic_04 },
        { "magic_05", TextureID::magic_05 },
        { "muzzle_01", TextureID::muzzle_01 },
        { "muzzle_02", TextureID::muzzle_02 },
        { "muzzle_03", TextureID::muzzle_03 },
        { "muzzle_04", TextureID::muzzle_04 },
        { "muzzle_05", TextureID::muzzle_05 },
        { "scorch_01", TextureID::scorch_01 },
        { "scorch_02", TextureID::scorch_02 },
        { "scorch_03", TextureID::scorch_03 },
        { "scratch_01", TextureID::scratch_01 },
        { "slash_01", TextureID::slash_01 },
        { "slash_02", TextureID::slash_02 },
        { "slash_03", TextureID::slash_03 },
        { "slash_04", TextureID::slash_04 },
        { "smoke_01", TextureID::smoke_01 },
        { "smoke_02", TextureID::smoke_02 },
        { "smoke_03", TextureID::smoke_03 },
        { "smoke_04", TextureID::smoke_04 },
        { "smoke_05", TextureID::smoke_05 },
        { "smoke_06", TextureID::smoke_06 },
        { "smoke_07", TextureID::smoke_07 },
        { "smoke_08", TextureID::smoke_08 },
        { "smoke_09", TextureID::smoke_09 },
        { "smoke_10", TextureID::smoke_10 },
        { "spark_01", TextureID::spark_01 },
        { "spark_02", TextureID::spark_02 },
        { "spark_03", TextureID::spark_03 },
        { "spark_04", TextureID::spark_04 },
        { "spark_05", TextureID::spark_05 },
        { "spark_06", TextureID::spark_06 },
        { "spark_07", TextureID::spark_07 },
        { "star_01", TextureID::star_01 },
        { "star_02", TextureID::star_02 },
        { "star_03", TextureID::star_03 },
        { "star_04", TextureID::star_04 },
        { "star_05", TextureID::star_05 },
        { "star_06", TextureID::star_06 },
        { "star_07", TextureID::star_07 },
        { "star_08", TextureID::star_08 },
        { "star_09", TextureID::star_09 }
    };
}

ParticleEditor::ParticleEditor(ParticleSystem* particleSystem)
    : particleSystem_(particleSystem)
{
}

void ParticleEditor::ShowEditor()
{
    if (ImGui::Begin("パーティクルエディター"))
    {
        // 1. ParticleTypeを選択
        static int selectedTypeIdx = static_cast<int>(ParticleType::Key);
        ImGui::Combo("Particle Type", &selectedTypeIdx, "None\0Key\0HitEffect\0");
        ParticleType type = static_cast<ParticleType>(selectedTypeIdx);

        // 選択されたタイプのプリセット名リストを動的に作成
        std::vector<const char*> presetNames;
        std::vector<std::string> presetNameStrings; // ImGui::Comboがchar*を要求するための一時的な保持用
        if (particleSystem_->definitions_.count(type)) {
            for (const auto& [name, def] : particleSystem_->definitions_.at(type)) {
                presetNameStrings.push_back(name);
            }
            for (const auto& name : presetNameStrings) {
                presetNames.push_back(name.c_str());
            }
        }
        // 2. Presetを選択
        static int selectedPresetIdx = 0;
        if (presetNames.empty()) {
            ImGui::Text("No presets available for this type.");
        }
        else
        {
            if (selectedPresetIdx >= presetNames.size())
            { // 範囲外アクセス防止
                selectedPresetIdx = 0;
            }
            ImGui::Combo("Preset", &selectedPresetIdx, presetNames.data(), (int)presetNames.size());

            const std::string& selectedPresetName = presetNames[selectedPresetIdx];
            auto& definition = particleSystem_->definitions_[type][selectedPresetName];
            auto& config = definition.particleConfig;
            auto& emitterConfig = definition.emitterConfig;

            if (ImGui::CollapsingHeader("Emitter Config"))
            {
                // 値が変更されたかを検出するためのフラグ
                bool valueChanged = false;

                // ImGuiの各ウィジェットが値を変更したら、valueChangedフラグを立てる
                valueChanged |= ImGui::DragFloat3("Position", &emitterConfig.position.x, 0.1f);
                valueChanged |= ImGui::DragFloat("Spawn Interval", &emitterConfig.spawnInterval, 0.01f, 0.01f, 10.0f);
                valueChanged |= ImGui::DragFloat("Particle Lifetime", &emitterConfig.lifetime, 0.01f, 0.0f, 10.0f);
                valueChanged |= ImGui::DragInt("Amount", &emitterConfig.amount, 1, 1, 100);

                // もし値が一つでも変更されていたら、ライブエミッターに設定を適用する
                if (valueChanged) {
                    particleSystem_->ApplyEmitterConfigToLiveEmitters(type, selectedPresetName);
                }
            }

            if (ImGui::CollapsingHeader("Particle Config"))
            {
                ImGui::DragFloat("Speed", &config.speed, 0.01f);
                ImGui::DragFloat("Gravity", &config.gravity, 0.01f);

                int selectedTextureIdx = 0;
                for (size_t i = 0; i < particleTextureList.size(); ++i) {
                    if (TextureHandle::Get(particleTextureList[i].second) == config.textureIndex) {
                        selectedTextureIdx = static_cast<int>(i);
                        break;
                    }
                }

                // 名前配列だけ作る
                std::vector<const char*> textureNameArray;
                for (const auto& pair : particleTextureList) {
                    textureNameArray.push_back(pair.first);
                }

                // Combo UI
                if (ImGui::Combo("Texture", &selectedTextureIdx, textureNameArray.data(), static_cast<int>(textureNameArray.size()))) {
                    config.textureIndex = TextureHandle::Get(particleTextureList[selectedTextureIdx].second);
                }
                ImGui::ColorEdit4("Base Color", &config.baseColor.x);
                // emitterRange
                ImGui::DragFloat3("Emitter Range", &config.emitterRange.x, 0.01f);

                // fadeOutEase interval
                ImGui::DragInt("FadeOut FrameCount", &config.fadeOutEase.frameCount_, 1);

                Vector4 startCol = Uint32ToColorVector(config.startColor);
                if (ImGui::ColorEdit4("Start Color", (float*)&startCol))
                {
                    config.startColor = ColorVectorToUint32(startCol);
                }

                Vector4 endCol = Uint32ToColorVector(config.endColor);
                if (ImGui::ColorEdit4("End Color", (float*)&endCol))
                {
                    config.endColor = ColorVectorToUint32(endCol);
                }

                // scaleEase interval
                ImGui::DragInt("Scale FrameCount", &config.scaleEase.frameCount_, 1);

                // scale
                ImGui::DragFloat3("Start Scale", &config.startScale.x, 0.01f);
                ImGui::DragFloat3("End Scale", &config.endScale.x, 0.01f);

            }

            if (ImGui::CollapsingHeader("State (Live Particles)"))
            {
                int i = 0;
                for (auto& particle : particleSystem_->particles_)
                {
                    if (particle.type != type) continue;

                    std::string label = "Particle[" + std::to_string(i++) + "]";
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        ImGui::DragFloat3("Position", &particle.transform->translation_.x, 0.1f);
                        ImGui::DragFloat("Speed", &particle.speed, 0.01f);
                        ImGui::ColorEdit4("Color", &particle.color.x);
                        ImGui::Checkbox("IsExist", &particle.isExist);

                        if (ImGui::Button("Apply Config Values"))
                        {
                            particle.speed = config.speed;
                            particle.color = config.baseColor;
                        }

                        ImGui::TreePop();
                    }
                }
            }

            if (ImGui::Button("Save"))
            {
                particleSystem_->SaveConfigToJson(type);

                std::string message = std::format("{}Particles.json saved", ParticleTypeToString(type));
                MessageBoxA(nullptr, message.c_str(), "Particles", 0);
            }

            ImGui::SameLine();


        }
    }
    ImGui::End();
}
