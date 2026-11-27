#include "pch.h"
#include "ParticleEditor.h"
#include "EditorWindowName.h"
#include "imGuiManager.h"
#include "TextureManager.h"
#include "ParticleEmitter.h"
#include "ParticleConfigManager.h" 
#include "ParticleSystem.h"
#include "Engine.h" 
#include "SRVManager.h"

namespace FS
{

namespace
{
// PropertyBinder::Draw と同じく、項目名を左の列に描き、次の入力欄を右の残りの幅いっぱいに伸ばす
// 戻り値は入力欄に渡すラベル。項目名は出さずに、今までと同じ文字列を ID にする
std::string LeftLabel(const char* label)
{
    std::string displayName = label;
    if (const size_t hiddenStart = displayName.find("##"); hiddenStart != std::string::npos)
    {
        displayName.resize(hiddenStart);
    }

    const ImGuiStyle& style = ImGui::GetStyle();
    const float fontSize = ImGui::GetFontSize();
    const float labelColumnWidth = std::clamp(ImGui::GetContentRegionAvail().x * 0.4f, fontSize * 5.0f, fontSize * 16.0f);
    const float startX = ImGui::GetCursorPosX();

    ImGui::Dummy(ImVec2(labelColumnWidth - style.ItemSpacing.x, ImGui::GetFrameHeight()));
    const ImVec2 labelMin = ImGui::GetItemRectMin();
    const ImVec2 labelMax = ImGui::GetItemRectMax();
    const ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
    // 入らない項目名は末尾を「...」にし、マウスを乗せると全文を出す
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(),
        ImVec2(labelMin.x, labelMin.y + style.FramePadding.y), labelMax, labelMax.x,
        displayName.c_str(), nullptr, &textSize);
    if (textSize.x > labelMax.x - labelMin.x && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", displayName.c_str());
    }

    ImGui::SameLine(startX + labelColumnWidth);
    ImGui::SetNextItemWidth(-FLT_MIN);
    return std::string("###") + label;
}
}

ParticleEditor::ParticleEditor(ParticleSystem* particleSystem, Engine* engine)
    : particleSystem_(particleSystem), engine_(engine)
{
}

void ParticleEditor::ShowEditor()
{
#ifdef ENABLE_IMGUI
    if (ImGui::Begin(EditorWindowName::kParticleEditor))
    {
        // プリセット名リスト生成
        std::vector<const char*> presetNames;
        std::vector<std::string> presetNameStrings;
        for (const auto& [name, def] : particleSystem_->definitions_)
        {
            presetNameStrings.push_back(name);
        }
        for (const auto& name : presetNameStrings)
        {
            presetNames.push_back(name.c_str());
        }

        // プリセットがない場合
        if (presetNames.empty())
        {
            ImGui::Text("利用可能なプリセットがない");
        }
        else
        {
            // 選択インデックス調整
            if (selectedPresetIdx_ >= presetNames.size())
            {
                selectedPresetIdx_ = 0;
            }

            // プリセット選択
            ImGui::Combo(LeftLabel("プリセット").c_str(), &selectedPresetIdx_, presetNames.data(), (int)presetNames.size());

            const std::string& selectedPresetName = presetNameStrings[selectedPresetIdx_];

            // 何度も押すので、スクロールしなくても見えるプリセットのすぐ下に置く
            if (ImGui::Button("セーブ"))
            {
                particleSystem_->configManager_->SaveParticleDefinitionToJson(selectedPresetName);
                saveMessageTimer_ = 3.0f;
            }
            if (saveMessageTimer_ > 0.0f)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s.json を保存", selectedPresetName.c_str());
                saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
            }
            ImGui::Separator();

            auto& definition = particleSystem_->definitions_[selectedPresetName];
            auto& config = definition.particleConfig;
            auto& emitterConfig = definition.emitterConfig;

            // パレット表示用の共通関数
            auto ShowTexturePalette = [&](const char* label, std::string& currentTexName, TextureType filterType)
                {
                    if (ImGui::TreeNode(label))
                    {
                        // 全テクスチャリストを取得
                        const auto& allTextures = TextureManager::GetInstance().GetAllTextures();

                        float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                        ImGuiStyle& style = ImGui::GetStyle();

                        // フィルタリングして表示
                        for (size_t i = 0; i < allTextures.size(); i++)
                        {
                            const auto& data = allTextures[i];

                            // 指定されたタイプ以外はスキップ
                            if (data.type != filterType) {
                                continue;
                            }

                            // GPUハンドル取得
                            auto gpuHandle = engine_->GetSRVManager()->GetSRVHandleGPU(data.handle);
                            ImTextureID imTexID = (ImTextureID)gpuHandle.ptr;

                            ImGui::PushID((int)i);

                            // 選択状態判定 
                            bool isSelected = (currentTexName == data.name);

                            if (isSelected) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                            }
                            else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                            }

                            // 画像ボタン
                            if (ImGui::ImageButton("TexBtn", imTexID, ImVec2(32, 32),
                                ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                            {
                                currentTexName = data.name;

                                // 変更された瞬間にハンドルを更新
                                config.RefreshResources();
                            }

                            ImGui::PopStyleColor();

                            // ツールチップ
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", data.name.c_str());
                                ImGui::Image(imTexID, ImVec2(128, 128));
                                ImGui::EndTooltip();
                            }

                            ImGui::PopID();

                            // 横並び計算
                            float lastButtonX2 = ImGui::GetItemRectMax().x;
                            float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + 32.0f;

                            if (nextButtonX2 < windowVisibleX2)
                            {
                                ImGui::SameLine();
                            }
                        }
                        ImGui::TreePop();
                    }
                };

            if (ImGui::CollapsingHeader("基本設定"))
            {
                bool intensityChanged = ImGui::DragFloat(LeftLabel("発光強度").c_str(), &config.intensity, 0.1f, 0.0f);

                ImGui::Separator();

                // コンボボックス表示用の配列
                const char* blendModeItems[] =
                {
                    "None",
                    "Normal",
                    "Add",
                };

                int currentBlendMode = static_cast<int>(config.blendMode);

                if (ImGui::Combo(LeftLabel("ブレンドモード").c_str(), &currentBlendMode, blendModeItems, IM_ARRAYSIZE(blendModeItems)))
                {
                    config.blendMode = static_cast<BlendMode>(currentBlendMode);
                }

            }

            // エミッター設定
            if (ImGui::CollapsingHeader("エミッター設定"))
            {
                bool valueChanged = false;

                // エミッター基本設定
                valueChanged |= ImGui::DragFloat3(LeftLabel("位置").c_str(), &emitterConfig.position.x, 0.1f);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat3(LeftLabel("追従オフセット").c_str(), &emitterConfig.followOffset.x, 0.1f);

                ImGui::Separator();
                bool intervalEdited = ImGui::DragFloat(LeftLabel("発生間隔 (秒)").c_str(), &emitterConfig.spawnInterval, 0.01f);
                valueChanged |= intervalEdited;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (emitterConfig.spawnInterval <= 0.01f)
                    {
                        emitterConfig.spawnInterval = 0.01f;
                    }
                    valueChanged = true;
                }

                if (ImGui::IsItemActive() && intervalEdited)
                {
                    if (emitterConfig.spawnInterval <= 0.0f)
                    {
                        valueChanged = false;
                    }
                }
                valueChanged |= ImGui::DragFloat(LeftLabel("生存時間").c_str(), &emitterConfig.lifetime, 0.01f, 0.0f, 10.0f);
                ImGui::Separator();
                valueChanged |= ImGui::DragInt(LeftLabel("発生数").c_str(), &emitterConfig.amount, 1, 0);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat(LeftLabel("再生時間").c_str(), &emitterConfig.duration, 0.001f);

                ImGui::Separator();
                valueChanged |= ImGui::Checkbox(LeftLabel("ループ").c_str(), &emitterConfig.looping);
                ImGui::Separator();
                valueChanged |= ImGui::Checkbox(LeftLabel("生成時に再生").c_str(), &emitterConfig.playOnAwake);

                // 設定変更をライブ反映
                if (valueChanged)
                {
                    ApplyEmitterConfigToLiveEmitters(selectedPresetName);
                }
            }

            // パーティクル設定
            if (ImGui::CollapsingHeader("パーティクル設定"))
            {
                // 速度モジュール
                if (ImGui::TreeNode("初速モジュール"))
                {
                    auto& vel = config.velocity;

                    ImGui::Checkbox(LeftLabel("有効##Velocity").c_str(), &vel.enabled);
                    ImGui::DragFloat(LeftLabel("速度").c_str(), &vel.speed, 0.01f, 0.0f, 1000.0f);
                    ImGui::Checkbox(LeftLabel("ランダム方向").c_str(), &vel.randomDirection);

                    if (vel.randomDirection)
                        ImGui::SliderFloat(LeftLabel("拡散角度").c_str(), &vel.angleRange, 0.0f, 360.0f, "%.0f 度");

                    ImGui::DragFloat3(LeftLabel("方向").c_str(), &vel.direction.x, 0.01f);

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 物理モジュール
                if (ImGui::TreeNode("物理モジュール"))
                {
                    auto& phys = config.physics;
                    ImGui::Checkbox(LeftLabel("有効##Physics").c_str(), &phys.enabled);
                    ImGui::DragFloat3(LeftLabel("重力").c_str(), &phys.gravity.x, 0.1f);
                    ImGui::DragFloat(LeftLabel("空気抵抗").c_str(), &phys.drag, 0.001f, 0.0f, 1.0f);

                    ImGui::Separator();
                    ImGui::Checkbox(LeftLabel("風の影響を受ける").c_str(), &phys.affectedByWind);
                    if (phys.affectedByWind)
                    {
                        ImGui::DragFloat(LeftLabel("風の影響度").c_str(), &phys.windInfluence, 0.01f, 0.0f, 10.0f);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 回転モジュール
                if (ImGui::TreeNode("回転モジュール"))
                {
                    auto& rot = config.rotation;

                    ImGui::Checkbox(LeftLabel("有効##Rotation").c_str(), &rot.enabled);
                    ImGui::Separator();
                    ImGui::Checkbox(LeftLabel("ビルボード").c_str(), &rot.isBillboard);
                    ImGui::Separator();

                    ImGui::Text("初期角度");
                    ImGui::DragFloat3(LeftLabel("最小##StartRotMin").c_str(), &rot.minStartRotation.x, 1.0f);
                    ImGui::DragFloat3(LeftLabel("最大##StartRotMax").c_str(), &rot.maxStartRotation.x, 1.0f);

                    ImGui::Separator();
                    ImGui::Text("回転速度");
                    if (rot.isBillboard)
                    {
                        ImGui::DragFloat(LeftLabel("最小速度 (2D)").c_str(), &rot.minAngularVelocity2D, 1.0f);
                        ImGui::DragFloat(LeftLabel("最大速度 (2D)").c_str(), &rot.maxAngularVelocity2D, 1.0f);
                    }
                    else
                    {
                        ImGui::DragFloat3(LeftLabel("最小速度 (3D)").c_str(), &rot.minAngularVelocity3D.x, 1.0f);
                        ImGui::DragFloat3(LeftLabel("最大速度 (3D)").c_str(), &rot.maxAngularVelocity3D.x, 1.0f);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 形状モジュール
                if (ImGui::TreeNode("エミッター形状モジュール"))
                {
                    auto& shape = config.shape;

                    const char* shapeTypes[] = { "点", "ボックス", "球" };
                    int currentShapeType = (int)shape.type;

                    if (ImGui::Combo(LeftLabel("形状タイプ").c_str(), &currentShapeType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
                        shape.type = (ShapeModule::Type)currentShapeType;

                    ImGui::Separator();

                    switch (shape.type)
                    {
                    case ShapeModule::Type::Box:
                        ImGui::DragFloat3(LeftLabel("ボックスサイズ").c_str(), &shape.boxSize.x, 0.1f);
                        break;
                    case ShapeModule::Type::Sphere:
                        ImGui::DragFloat3(LeftLabel("半径").c_str(), &shape.radius.x, 0.1f);
                        ImGui::Checkbox(LeftLabel("縁から放出").c_str(), &shape.emitFromEdge);
                        break;
                    case ShapeModule::Type::Point:
                        break;
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // テクスチャシート
                ShowTexturePalette("テクスチャモジュール", config.textureSheet.textureName, TextureType::Particle);

                ImGui::Separator();

                // 色変化モジュール
                if (ImGui::TreeNode("生存期間中の色"))
                {
                    auto& color = config.colorOverLifetime;

                    ImGui::Checkbox(LeftLabel("有効##Color").c_str(), &color.enabled);

                    const char* modes[] = { "単一", "ランダム2種" };
                    int mode = (int)color.mode;
                    if (ImGui::Combo(LeftLabel("モード").c_str(), &mode, modes, IM_ARRAYSIZE(modes)))
                        color.mode = (ColorOverLifetimeModule::Mode)mode;

                    // グラデーション1
                    Vector4 start1 = Math::Uint32ToColorVector(color.startColor);
                    if (ImGui::ColorEdit4(LeftLabel("開始色 1").c_str(), &start1.x))
                        color.startColor = Math::ColorVectorToUint32(start1);

                    Vector4 end1 = Math::Uint32ToColorVector(color.endColor);
                    if (ImGui::ColorEdit4(LeftLabel("終了色 1").c_str(), &end1.x))
                        color.endColor = Math::ColorVectorToUint32(end1);

                    // グラデーション2
                    if (color.mode == ColorOverLifetimeModule::Mode::RandomBetweenTwo)
                    {
                        Vector4 start2 = Math::Uint32ToColorVector(color.startColor2);
                        if (ImGui::ColorEdit4(LeftLabel("開始色 2").c_str(), &start2.x))
                            color.startColor2 = Math::ColorVectorToUint32(start2);

                        Vector4 end2 = Math::Uint32ToColorVector(color.endColor2);
                        if (ImGui::ColorEdit4(LeftLabel("終了色 2").c_str(), &end2.x))
                            color.endColor2 = Math::ColorVectorToUint32(end2);
                    }

                    ImGui::Separator();

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // サイズ変化モジュール
                if (ImGui::TreeNode("生存期間中のサイズ"))
                {
                    auto& size = config.sizeOverLifetime;

                    ImGui::Checkbox(LeftLabel("有効##Size").c_str(), &size.enabled);
                    ImGui::DragFloat3(LeftLabel("開始サイズ").c_str(), &size.startScale.x, 0.01f);
                    ImGui::DragFloat3(LeftLabel("終了サイズ").c_str(), &size.endScale.x, 0.01f);

                    ImGui::Separator();
                    ImGui::Checkbox(LeftLabel("振動").c_str(), &size.oscillate);
                    if (size.oscillate)
                        ImGui::DragFloat(LeftLabel("周波数").c_str(), &size.frequency, 0.1f, 0.0f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::Separator();
                if (ImGui::TreeNode("引力モジュール"))
                {
                    auto& attraction = config.attraction;

                    ImGui::Checkbox(LeftLabel("有効##Attraction").c_str(), &attraction.enabled);

                    if (attraction.enabled)
                    {
                        ImGui::DragFloat(LeftLabel("引力の強さ").c_str(), &attraction.strength, 0.1f, -500.0f, 500.0f);

                        ImGui::Separator();
                        ImGui::Text("静的ターゲット (ターゲット未設定時)");
                        ImGui::DragFloat3(LeftLabel("座標##AttractionTarget").c_str(), &attraction.target.x, 0.1f);

                        ImGui::Separator();
                        ImGui::Text("動的ターゲット (SetTargetToFollow使用時)");
                        ImGui::DragFloat3(LeftLabel("オフセット##AttractionOffset").c_str(), &attraction.offset.x, 0.1f);
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();

                if (ImGui::TreeNode("トレイルモジュール"))
                {
                    auto& trail = config.trail;

                    ImGui::Checkbox(LeftLabel("有効##Trail").c_str(), &trail.enabled);

                    if (trail.enabled)
                    {
                        ImGui::DragFloat(LeftLabel("太さ").c_str(), &trail.width, 0.1f, 0.1f, 10.0f);
                        ImGui::DragFloat(LeftLabel("発光強度").c_str(), &trail.emissiveIntensity, 0.1f, 0.0f, 100.0f);
                        ImGui::DragFloat(LeftLabel("寿命").c_str(), &trail.lifetime, 0.1f, 0.1f, 5.0f);
                        ImGui::DragFloat(LeftLabel("最小頂点距離").c_str(), &trail.minVertexDistance, 0.01f, 0.01f, 10.0f);

                        ImGui::Separator();

                        // メインテクスチャ選択
                        ShowTexturePalette("トレイルテクスチャ選択", trail.textureName, TextureType::Particle);

                        ImGui::Separator();

                        const char* jitterModes[] = { "滑らか", "ランダム" };
                        int currentJitter = static_cast<int>(trail.jitterMode);
                        if (ImGui::Combo(LeftLabel("揺れタイプ").c_str(), &currentJitter, jitterModes, IM_ARRAYSIZE(jitterModes)))
                        {
                            trail.jitterMode = static_cast<JitterMode>(currentJitter);
                        }

                        ImGui::Text("ジッター (形状変形)");
                        ImGui::DragFloat(LeftLabel("強さ").c_str(), &trail.jitterStrength, 0.1f, 0.0f, 50.0f);
                        ImGui::DragFloat(LeftLabel("周波数").c_str(), &trail.jitterFrequency, 0.1f, 0.1f, 100.0f);
                        ImGui::DragFloat(LeftLabel("変動速度").c_str(), &trail.jitterSpeed, 0.1f, 0.0f, 100.0f);
                        ImGui::DragFloat(LeftLabel("位相 (位置ずらし)").c_str(), &trail.jitterPhase, 0.01f, -10.0f, 10.0f);

                        ImGui::Separator();

                        ImGui::Text("太さの変化");
                        ImGui::DragFloat(LeftLabel("先端スケール").c_str(), &trail.headWidthScale, 0.01f, 0.0f, 5.0f);
                        ImGui::DragFloat(LeftLabel("末尾スケール").c_str(), &trail.tailWidthScale, 0.01f, 0.0f, 5.0f);

                        ImGui::Separator();

                        const char* alignModes[] = { "カメラ向き", "回転追従" };
                        int currentAlign = static_cast<int>(trail.alignment);
                        if (ImGui::Combo(LeftLabel("向きの制御").c_str(), &currentAlign, alignModes, IM_ARRAYSIZE(alignModes)))
                        {
                            trail.alignment = static_cast<TrailAlignment>(currentAlign);
                        }

                        ImGui::Separator();

                        ImGui::Text("カラー推移");
                        ImGui::ColorEdit4(LeftLabel("開始色 (根元)").c_str(), &trail.startColor.x);
                        ImGui::ColorEdit4(LeftLabel("終了色 (先端)").c_str(), &trail.endColor.x);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("ノイズモジュール"))
                {
                    auto& noise = config.noise;

                    ImGui::Checkbox(LeftLabel("有効##Noise").c_str(), &noise.enabled);
                    if (noise.enabled)
                    {
                        const char* typeNames[] = { "Simplex", "Curl " };
                        int currentType = static_cast<int>(noise.type);
                        if (ImGui::Combo(LeftLabel("タイプ").c_str(), &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
                        {
                            noise.type = static_cast<NoiseModule::NoiseType>(currentType);
                        }

                        ImGui::DragFloat(LeftLabel("強度").c_str(), &noise.strength, 0.1f, 0.0f, 100.0f);

                        ImGui::DragFloat(LeftLabel("周波数").c_str(), &noise.frequency, 0.002f, 0.001f, 0.5f, "%.3f");
                        ImGui::DragFloat(LeftLabel("スクロール速度").c_str(), &noise.scrollSpeed, 0.01f, 0.0f, 10.0f);

                        if (noise.type == NoiseModule::NoiseType::Simplex)
                        {
                            ImGui::Checkbox(LeftLabel("XYZ軸で分離").c_str(), &noise.separateAxes);
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
    }

    ImGui::End();
#endif
}

void ParticleEditor::ApplyEmitterConfigToLiveEmitters(const std::string& presetName)
{
    // 指定プリセットのエミッター設定を取得
    const auto& emitterConfig = particleSystem_->definitions_.at(presetName).emitterConfig;

    // すべてのライブエミッターをチェック
    for (auto& emitter : particleSystem_->emitters_)
    {
        // プリセット名が一致するエミッターに設定を適用
        if (emitter->presetName_ == presetName)
        {
            emitter->position_ = emitterConfig.position;
            emitter->spawnInterval_ = emitterConfig.spawnInterval;
            emitter->lifetime_ = emitterConfig.lifetime;
            emitter->amount_ = emitterConfig.amount;
            emitter->duration_ = emitterConfig.duration;
            emitter->followOffset_ = emitterConfig.followOffset;

            // ループ設定を更新し、停止中だったエミッターを再生
            bool wasStopped = !emitter->isPlaying_;
            emitter->looping_ = emitterConfig.looping;
            if (wasStopped && emitter->looping_)
                emitter->Play();
        }
    }
}

}