#include "ParticleEditor.h"
#include "imGuiManager.h"
#include "TextureHandle.h"
#include "ParticleTextureHandle.h"
#include "ParticleEmitter.h"
#include "ParticleConfigManager.h" 

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

                    const auto& allDefinitions = ParticleTextureHandle::GetDefinitions();
                    std::vector<const char*> textureNameArray;
                    std::vector<ParticleTextureID> idArray;

                    for (const auto& def : allDefinitions)
                    {
                        const char* path = def.path;
                        const char* filename = path; // デフォルトはパス全体

                        // 最後の '/' を探す
                        const char* lastSlash = strrchr(path, '/');
                        // 最後の '\' を探す
                        const char* lastBackslash = strrchr(path, '\\');

                        const char* separator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

                        if (lastSlash > separator)
                        {
                            separator = lastSlash;
                        }
                        if (lastBackslash > separator) 
                        {
                            separator = lastBackslash;
                        }

                        if (separator != nullptr)
                        {
                            filename = separator + 1;
                        }

                        textureNameArray.push_back(filename); 

                        idArray.push_back(def.id);
                    }

                    int selectedTextureIdx = 0;

                    // ローカル変数を探す
                    for (size_t i = 0; i < idArray.size(); ++i)
                    {
                        if (ParticleTextureHandle::Get(idArray[i]) == texSheet.textureHandle)
                        {
                            selectedTextureIdx = static_cast<int>(i);
                            break;
                        }
                    }
                    if (ImGui::Combo("テクスチャ", &selectedTextureIdx, textureNameArray.data(), static_cast<int>(textureNameArray.size())))
                    {
                        texSheet.textureHandle = ParticleTextureHandle::Get(idArray[selectedTextureIdx]);
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

                    const char* modes[] = { "単一グラデーション", "2グラデーションからランダム" };
                    int currentMode = static_cast<int>(colorModule.mode);
                    if (ImGui::Combo("モード", &currentMode, modes, IM_ARRAYSIZE(modes)))
                    {
                        colorModule.mode = static_cast<ColorOverLifetimeModule::Mode>(currentMode);
                    }

                    // --- グラデーション 1 ---
                    ImGui::Text("グラデーション 1");
                    Vector4 startCol = Math::Uint32ToColorVector(colorModule.startColor);
                    if (ImGui::ColorEdit4("開始色 1", &startCol.x)) {
                        colorModule.startColor = Math::ColorVectorToUint32(startCol);
                    }
                    Vector4 endCol = Math::Uint32ToColorVector(colorModule.endColor);
                    if (ImGui::ColorEdit4("終了色 1", &endCol.x)) {
                        colorModule.endColor = Math::ColorVectorToUint32(endCol);
                    }

                    // ★ モードが "RandomBetweenTwo" の場合のみグラデーション2を表示
                    if (colorModule.mode == ColorOverLifetimeModule::Mode::RandomBetweenTwo)
                    {
                        ImGui::Separator();
                        ImGui::Text("グラデーション 2");

                        Vector4 startCol2 = Math::Uint32ToColorVector(colorModule.startColor2);
                        if (ImGui::ColorEdit4("開始色 2", &startCol2.x)) {
                            colorModule.startColor2 = Math::ColorVectorToUint32(startCol2);
                        }
                        Vector4 endCol2 = Math::Uint32ToColorVector(colorModule.endColor2);
                        if (ImGui::ColorEdit4("終了色 2", &endCol2.x)) {
                            colorModule.endColor2 = Math::ColorVectorToUint32(endCol2);
                        }
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