#include "ParticleEditor.h"
#include "imGuiManager.h"
#include "TextureHandle.h"
#include "ParticleEmitter.h"
#include "ParticleConfigManager.h" 

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
        { "star_09", TextureID::star_09 },
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
        // definitions_マップのキーからプリセット名リストを直接作成
        std::vector<const char*> presetNames;
        std::vector<std::string> presetNameStrings;
        for (const auto& [name, def] : particleSystem_->definitions_) {
            presetNameStrings.push_back(name);
        }
        for (const auto& name : presetNameStrings) {
            presetNames.push_back(name.c_str());
        }

        if (presetNames.empty())
        {
            ImGui::Text("利用可能なプリセットがありません。");
        }
        else
        {
            if (selectedPresetIdx_ >= presetNames.size()) {
                selectedPresetIdx_ = 0;
            }
            ImGui::Combo("プリセット", &selectedPresetIdx_, presetNames.data(), (int)presetNames.size());

            const std::string& selectedPresetName = presetNames[selectedPresetIdx_];
            auto& definition = particleSystem_->definitions_[selectedPresetName];
            auto& config = definition.particleConfig;
            auto& emitterConfig = definition.emitterConfig;

            if (ImGui::CollapsingHeader("エミッター設定"))
            {
                // 値が変更されたかを検出するためのフラグ
                bool valueChanged = false;

                // ImGuiの各ウィジェットが値を変更したら、valueChangedフラグを立てる
                valueChanged |= ImGui::DragFloat3("位置", &emitterConfig.position.x, 0.1f);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat("発生間隔 (秒)", &emitterConfig.spawnInterval, 0.01f, 0.01f, 10.0f);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat("パーティクルの生存時間", &emitterConfig.lifetime, 0.01f, 0.0f, 10.0f);
                ImGui::Separator();
                valueChanged |= ImGui::DragInt("一度の発生数", &emitterConfig.amount, 1, 0);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat("再生時間", &emitterConfig.duration, 0.1f, -1.0f, 300.0f, "%.1f 秒");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("エミッターがパーティクルを放出し続ける時間\n-1で無限");
                }
                ImGui::Separator();
                valueChanged |= ImGui::Checkbox("ループ", &emitterConfig.looping);
                ImGui::Separator();
                valueChanged |= ImGui::Checkbox("生成時に再生", &emitterConfig.playOnAwake);

                // もし値が一つでも変更されていたら、ライブエミッターに設定を適用する
                if (valueChanged)
                {
                    ApplyEmitterConfigToLiveEmitters(selectedPresetName);
                }
            }

            if (ImGui::CollapsingHeader("パーティクル設定"))
            {
                if (ImGui::TreeNode("初速モジュール"))
                {
                    // config.velocityへの参照
                    auto& vel = config.velocity;

                    ImGui::Checkbox("有効##Velocity", &vel.enabled);
                    ImGui::DragFloat("速度", &vel.speed, 0.01f, 0.0f, 1000.0f);
                    ImGui::Checkbox("ランダムな方向", &vel.randomDirection);

                    // randomDirectionがtrueのときだけangleRangeを表示
                    if (vel.randomDirection)
                    {
                        ImGui::SliderFloat("拡散角度", &vel.angleRange, 0.0f, 360.0f, "%.0f 度");
                    }

                    // randomDirectionがfalseのときは固定方向、trueのときは中心方向として使う
                    ImGui::DragFloat3("放出方向", &vel.direction.x, 0.01f);

                    ImGui::TreePop();
                }
                ImGui::Separator();

                if (ImGui::TreeNode("物理モジュール"))
                {
                    auto& phys = config.physics;
                    ImGui::Checkbox("有効##Physics", &phys.enabled);
                    ImGui::DragFloat3("重力", &phys.gravity.x, 0.1f);
                    ImGui::DragFloat("空気抵抗", &phys.drag, 0.001f, 0.0f, 1.0f);
                    ImGui::TreePop();
                }

                ImGui::Separator();
                if (ImGui::TreeNode("回転モジュール"))
                {
                    auto& rot = config.rotation;
                    ImGui::Checkbox("有効##Rotation", &rot.enabled);
                    ImGui::Separator();
                    ImGui::Checkbox("ビルボード", &rot.isBillboard);
                    ImGui::Separator();
                    ImGui::DragFloat3("初期角度 (3D)", &rot.orientation3D.x, 1.0f, -360.0f, 360.0f, "%.1f 度");
                    ImGui::Separator();
                    if (rot.isBillboard)
                    {
                        // ビルボードが有効な場合のUI
                        ImGui::Checkbox("開始角度をランダムに", &rot.randomStartRotation);
                        ImGui::DragFloat("回転速度 (2D)", &rot.angularVelocity2D, 1.0f, 0.0f, 0.0f, "%.1f 度/秒");
                    }
                    else
                    {
                        ImGui::DragFloat3("回転速度 (3D)", &rot.angularVelocity3D.x, 1.0f, 0.0f, 0.0f, "%.1f 度/秒");
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();

                if (ImGui::TreeNode("形状モジュール"))
                {
                    auto& shape = config.shape;

                    // 形状タイプを選択するコンボボックス
                    const char* shapeTypes[] = { "点", "ボックス", "球" };
                    int currentShapeType = static_cast<int>(shape.type);
                    if (ImGui::Combo("形状タイプ", &currentShapeType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
                    {
                        shape.type = static_cast<ShapeModule::Type>(currentShapeType);
                    }
                    ImGui::Separator();

                    // 選択された形状に応じて、関連するUIのみを表示
                    switch (shape.type)
                    {
                    case ShapeModule::Type::Box:
                        ImGui::DragFloat3("ボックスのサイズ", &shape.boxSize.x, 0.1f);
                        break;
                    case ShapeModule::Type::Sphere:
                        ImGui::DragFloat3("半径", &shape.radius.x, 0.1f);
                        ImGui::Separator();
                        ImGui::Checkbox("縁から放出", &shape.emitFromEdge);
                        break;
                    case ShapeModule::Type::Point:
                        break;
                    }

                    ImGui::Separator();

                    ImGui::TreePop();
                }
                ImGui::Separator();

                if (ImGui::TreeNode("テクスチャシートモジュール"))
                {
                    auto& texSheet = config.textureSheet;

                    ImGui::Checkbox("有効##Texture", &texSheet.enabled);

                    // 現在のインデックスを探す
                    for (size_t i = 0; i < particleTextureList.size(); ++i)
                    {
                        if (TextureHandle::Get(particleTextureList[i].second) == texSheet.textureHandle)
                        {
                            selectedTextureIdx_ = static_cast<int>(i);
                            break;
                        }
                    }

                    // 表示用の名前配列を作る
                    std::vector<const char*> textureNameArray;
                    for (const auto& pair : particleTextureList)
                    {
                        textureNameArray.push_back(pair.first);
                    }

                    // 選択されたらハンドルを更新
                    if (ImGui::Combo("テクスチャ", &selectedTextureIdx_, textureNameArray.data(), static_cast<int>(textureNameArray.size())))
                    {
                        texSheet.textureHandle = TextureHandle::Get(particleTextureList[selectedTextureIdx_].second);
                    }

                    ImGui::DragInt("横の分割数", &texSheet.tilesX, 1, 1, 16);
                    ImGui::DragInt("縦の分割数", &texSheet.tilesY, 1, 1, 16);
                    ImGui::DragFloat("再生速度 (フレーム/秒)", &texSheet.framesPerSecond, 0.1f, 0.0f, 60.0f);
                    ImGui::Checkbox("ループ##TextureLoop", &texSheet.looping);

                    ImGui::TreePop();
                }
                ImGui::Separator();

                // ColorOverLifetimeModuleのUI
                if (ImGui::TreeNode("生存期間中の色モジュール"))
                {
                    auto& colorModule = config.colorOverLifetime;

                    ImGui::Checkbox("有効##Color", &colorModule.enabled);

                    // 開始色
                    Vector4 startCol = Math::Uint32ToColorVector(colorModule.startColor);
                    if (ImGui::ColorEdit4("開始色", &startCol.x)) {
                        colorModule.startColor = Math::ColorVectorToUint32(startCol);
                    }

                    // 終了色
                    Vector4 endCol = Math::Uint32ToColorVector(colorModule.endColor);
                    if (ImGui::ColorEdit4("終了色", &endCol.x)) {
                        colorModule.endColor = Math::ColorVectorToUint32(endCol);
                    }

                    // イージングタイプの選択
                    // (EasingTypeのEnumに対応する文字列配列をどこかで定義しておく)
                    //const char* easingTypes[] = { "Linear", "InSine", "OutSine", /* ... */ };
                    //int currentEasing = static_cast<int>(colorModule.easing.GetEasingType());
                    //if (ImGui::Combo("Easing Type##Color", &currentEasing, easingTypes, IM_ARRAYSIZE(easingTypes))) {
                    //    colorModule.easing.SetEasing(static_cast<EasingType>(currentEasing));
                    //}

                    ImGui::TreePop();
                }
                ImGui::Separator();

                // SizeOverLifetimeModuleのUI
                if (ImGui::TreeNode("生存期間中のサイズモジュール"))
                {
                    auto& sizeModule = config.sizeOverLifetime; // ショートカット

                    ImGui::Checkbox("有効##Size", &sizeModule.enabled);
                    ImGui::DragFloat3("開始サイズ", &sizeModule.startScale.x, 0.01f);
                    ImGui::DragFloat3("終了サイズ", &sizeModule.endScale.x, 0.01f);
                    ImGui::Separator();
                    ImGui::Checkbox("振動させる", &sizeModule.oscillate);
                    if (sizeModule.oscillate)
                    {
                        ImGui::DragFloat("周波数", &sizeModule.frequency, 0.1f, 0.0f, 100.0f);
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();

                // VortexModuleのUI
                if (ImGui::TreeNode("渦モジュール"))
                {
                    auto& vortex = config.vortex;
                    ImGui::Checkbox("有効##Vortex", &vortex.enabled);
                    ImGui::DragFloat3("中心座標", &vortex.center.x, 0.1f);
                    ImGui::DragFloat("回転速度", &vortex.rotationSpeed, 1.0f, -1000.0f, 1000.0f, "%.0f 度/秒");
                    ImGui::DragFloat("公転速度", &vortex.orbitalSpeed, 0.1f, -100.0f, 100.0f);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("中心へ向かう/離れる速度。\nマイナス値で中心から離れます。");
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();

                // AttractionModuleのUI
                if (ImGui::TreeNode("引力モジュール"))
                {
                    auto& attraction = config.attraction;

                    ImGui::Checkbox("有効##Attraction", &attraction.enabled);
                    ImGui::DragFloat3("目標地点", &attraction.target.x, 0.1f);
                    ImGui::DragFloat("強さ", &attraction.strength, 0.1f, 0.0f, 1000.0f);

                    ImGui::TreePop();
                }
                ImGui::Separator();
            }

            if (ImGui::Button("セーブ"))
            {
                particleSystem_->configManager_->SaveParticleDefinitionToJson(selectedPresetName);
                // 表示するメッセージを作成する
                std::string message = std::format("{}.json saved", selectedPresetName);
                MessageBoxA(nullptr, message.c_str(), "Save Confirmation", MB_OK);
            }

            ImGui::SameLine();


        }

    }
    ImGui::End();

}

void ParticleEditor::ApplyEmitterConfigToLiveEmitters(const std::string& presetName)
{
    // 更新する設定をpresetNameで特定
    const auto& emitterConfig = particleSystem_->definitions_.at(presetName).emitterConfig;

    // 全てのライブエミッターをループ
    for (auto& emitter : particleSystem_->emitters_)
    {
        // プリセット名が一致するエミッターを見つける
        if (emitter->presetName_ == presetName)
        {
            // インスタンスの値をemitterConfigの値で上書きする
            emitter->position_ = emitterConfig.position;
            emitter->spawnInterval_ = emitterConfig.spawnInterval;
            emitter->lifetime_ = emitterConfig.lifetime;
            emitter->amount_ = emitterConfig.amount;
            emitter->duration_ = emitterConfig.duration;

            // 設定を適用する前の状態を記憶
            bool wasStopped = !emitter->isPlaying_;
            emitter->looping_ = emitterConfig.looping;

            if (wasStopped && emitter->looping_)
            {
                emitter->Play();
            }
        }
    }
}