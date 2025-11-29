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
            ImGui::Text("利用可能なプリセットがありません。");
        }
        else
        {
            // 選択インデックス調整
            if (selectedPresetIdx_ >= presetNames.size()) 
            {
                selectedPresetIdx_ = 0;
            }

            // プリセット選択
            ImGui::Combo("プリセット", &selectedPresetIdx_, presetNames.data(), (int)presetNames.size());

            const std::string& selectedPresetName = presetNameStrings[selectedPresetIdx_];
            auto& definition = particleSystem_->definitions_[selectedPresetName];
            auto& config = definition.particleConfig;
            auto& emitterConfig = definition.emitterConfig;

            if (ImGui::CollapsingHeader("基本設定"))
            {
                bool intensityChanged = ImGui::DragFloat("発光強度", &config.intensity, 0.1f, 0.0f);

                ImGui::Separator();

                // コンボボックス表示用の配列
                const char* blendModeItems[] =
                {
                    "None",
                    "Normal",
                    "Add",
                    "Subtract",
                    "Multiply",
                    "Screen",
                    "Exclusion"
                };

                int currentBlendMode = static_cast<int>(config.blendMode);

                if (ImGui::Combo("ブレンドモード", &currentBlendMode, blendModeItems, IM_ARRAYSIZE(blendModeItems)))
                {
                    config.blendMode = static_cast<BlendMode>(currentBlendMode);
                }

            }

            // エミッター設定
            if (ImGui::CollapsingHeader("エミッター設定"))
            {
                bool valueChanged = false;

                // エミッター基本設定
                valueChanged |= ImGui::DragFloat3("位置", &emitterConfig.position.x, 0.1f);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat3("追従オフセット", &emitterConfig.followOffset.x, 0.1f);

                ImGui::Separator();
                bool intervalEdited = ImGui::DragFloat("発生間隔 (秒)", &emitterConfig.spawnInterval, 0.01f);
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
                valueChanged |= ImGui::DragFloat("生存時間", &emitterConfig.lifetime, 0.01f, 0.0f, 10.0f);
                ImGui::Separator();
                valueChanged |= ImGui::DragInt("発生数", &emitterConfig.amount, 1, 0);
                ImGui::Separator();
                valueChanged |= ImGui::DragFloat("再生時間", &emitterConfig.duration, 0.1f, -1.0f, 300.0f, "%.1f 秒");

                ImGui::Separator();
                valueChanged |= ImGui::Checkbox("ループ", &emitterConfig.looping);
                ImGui::Separator();
                valueChanged |= ImGui::Checkbox("生成時に再生", &emitterConfig.playOnAwake);

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

                    ImGui::Checkbox("有効##Velocity", &vel.enabled);
                    ImGui::DragFloat("速度", &vel.speed, 0.01f, 0.0f, 1000.0f);
                    ImGui::Checkbox("ランダム方向", &vel.randomDirection);

                    if (vel.randomDirection)
                        ImGui::SliderFloat("拡散角度", &vel.angleRange, 0.0f, 360.0f, "%.0f 度");

                    ImGui::DragFloat3("方向", &vel.direction.x, 0.01f);

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 物理モジュール
                if (ImGui::TreeNode("物理モジュール"))
                {
                    auto& phys = config.physics;
                    ImGui::Checkbox("有効##Physics", &phys.enabled);
                    ImGui::DragFloat3("重力", &phys.gravity.x, 0.1f);
                    ImGui::DragFloat("空気抵抗", &phys.drag, 0.001f, 0.0f, 1.0f);
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 回転モジュール
                if (ImGui::TreeNode("回転モジュール"))
                {
                    auto& rot = config.rotation;

                    ImGui::Checkbox("有効##Rotation", &rot.enabled);
                    ImGui::Separator();
                    ImGui::Checkbox("ビルボード", &rot.isBillboard);
                    ImGui::Separator();

                    ImGui::Text("初期角度");
                    ImGui::DragFloat3("最小##StartRotMin", &rot.minStartRotation.x, 1.0f);
                    ImGui::DragFloat3("最大##StartRotMax", &rot.maxStartRotation.x, 1.0f);

                    ImGui::Separator();
                    ImGui::Text("回転速度");
                    if (rot.isBillboard)
                    {
                        ImGui::DragFloat("最小速度 (2D)", &rot.minAngularVelocity2D, 1.0f);
                        ImGui::DragFloat("最大速度 (2D)", &rot.maxAngularVelocity2D, 1.0f);
                    }
                    else
                    {
                        ImGui::DragFloat3("最小速度 (3D)", &rot.minAngularVelocity3D.x, 1.0f);
                        ImGui::DragFloat3("最大速度 (3D)", &rot.maxAngularVelocity3D.x, 1.0f);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 形状モジュール
                if (ImGui::TreeNode("形状モジュール"))
                {
                    auto& shape = config.shape;

                    const char* shapeTypes[] = { "点", "ボックス", "球" };
                    int currentShapeType = (int)shape.type;

                    if (ImGui::Combo("形状タイプ", &currentShapeType, shapeTypes, IM_ARRAYSIZE(shapeTypes)))
                        shape.type = (ShapeModule::Type)currentShapeType;

                    ImGui::Separator();

                    switch (shape.type)
                    {
                    case ShapeModule::Type::Box:
                        ImGui::DragFloat3("ボックスサイズ", &shape.boxSize.x, 0.1f);
                        break;
                    case ShapeModule::Type::Sphere:
                        ImGui::DragFloat3("半径", &shape.radius.x, 0.1f);
                        ImGui::Checkbox("縁から放出", &shape.emitFromEdge);
                        break;
                    case ShapeModule::Type::Point:
                        break;
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // テクスチャシート
                if (ImGui::TreeNode("テクスチャモジュール"))
                {
                    auto& texSheet = config.textureSheet;
                  
                    // テクスチャ一覧取得
                    const auto& allDefinitions = ParticleTextureHandle::GetDefinitions();
                    std::vector<const char*> textureNameArray;
                    std::vector<ParticleTextureID> idArray;

                    for (const auto& def : allDefinitions)
                    {
                        const char* path = def.path;
                        const char* filename = path;

                        const char* lastSlash = strrchr(path, '/');
                        const char* lastBackslash = strrchr(path, '\\');
                        const char* separator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

                        if (separator)
                            filename = separator + 1;

                        textureNameArray.push_back(filename);
                        idArray.push_back(def.id);
                    }

                    int selectedTextureIdx = 0;
                    for (size_t i = 0; i < idArray.size(); ++i)
                    {
                        if (ParticleTextureHandle::Get(idArray[i]) == texSheet.textureHandle)
                        {
                            selectedTextureIdx = (int)i;
                            break;
                        }
                    }

                    if (ImGui::Combo("テクスチャ", &selectedTextureIdx,
                        textureNameArray.data(),
                        (int)textureNameArray.size()))
                    {
                        texSheet.textureHandle = ParticleTextureHandle::Get(idArray[selectedTextureIdx]);
                    }


                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("衝突モジュール"))
                {
                    auto& col = config.collision;
                    ImGui::Checkbox("有効##Collision", &col.enabled);

                    if (col.enabled)
                    {
                        const char* typeItems[] = { "平面", "ワールドオブジェクト" };
                        int typeIdx = static_cast<int>(col.type);
                        if (ImGui::Combo("タイプ", &typeIdx, typeItems, IM_ARRAYSIZE(typeItems)))
                            col.type = static_cast<CollisionModule::Type>(typeIdx);

                        ImGui::Separator();

                        if (col.type == CollisionModule::Type::Plane)
                        {
                            ImGui::Text("平面設定");
                            ImGui::DragFloat3("位置", &col.plane.point.x, 0.1f);
                            ImGui::DragFloat3("法線", &col.plane.normal.x, 0.01f, -1.0f, 1.0f);
                            if (ImGui::Button("法線の正規化")) {
                                col.plane.normal = col.plane.normal.Normalize();
                            }

                            // ギズモ描画 (デバッグ用)
                            // 緑色のグリッドなどを描画して平面を可視化する
                            // DebugDraw::DrawGrid(col.plane.point, col.plane.normal, 10.0f, Color::Green);
                        }
                        else
                        {
                            ImGui::Text("ワールドオブジェクト設定");
                            const char* shapes[] = { "球体", "箱" };
                            int shapeIdx = static_cast<int>(col.worldObj.shape);
                            if (ImGui::Combo("形状", &shapeIdx, shapes, IM_ARRAYSIZE(shapes)))
                                col.worldObj.shape = static_cast<CollisionModule::WorldObject::Shape>(shapeIdx);

                            ImGui::DragFloat3("中心", &col.worldObj.center.x, 0.1f);

                            if (col.worldObj.shape == CollisionModule::WorldObject::Shape::Sphere)
                                ImGui::DragFloat("半径", &col.worldObj.scale.x, 0.1f);
                            else
                                ImGui::DragFloat3("サイズ", &col.worldObj.scale.x, 0.1f);

                            // ギズモ描画
                            // if (shape == Sphere) DebugDraw::DrawWireSphere(center, radius, Color::Red);
                            // else DebugDraw::DrawWireBox(center, size, Color::Red);
                        }

                        ImGui::Separator();

                        ImGui::Text("物理特性");
                        ImGui::DragFloat("反発", &col.bounce, 0.01f, 0.0f, 2.0f);
                        ImGui::DragFloat("減衰", &col.dampen, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("摩擦", &col.friction, 0.01f, 0.0f, 1.0f);
                        ImGui::DragFloat("寿命減少", &col.lifeLoss, 0.01f, 0.0f, 1.0f);
                    }
                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 色変化モジュール
                if (ImGui::TreeNode("生存期間中の色"))
                {
                    auto& color = config.colorOverLifetime;

                    ImGui::Checkbox("有効##Color", &color.enabled);

                    const char* modes[] = { "単一", "ランダム2種" };
                    int mode = (int)color.mode;
                    if (ImGui::Combo("モード", &mode, modes, IM_ARRAYSIZE(modes)))
                        color.mode = (ColorOverLifetimeModule::Mode)mode;

                    // グラデーション1
                    Vector4 start1 = Math::Uint32ToColorVector(color.startColor);
                    if (ImGui::ColorEdit4("開始色 1", &start1.x))
                        color.startColor = Math::ColorVectorToUint32(start1);

                    Vector4 end1 = Math::Uint32ToColorVector(color.endColor);
                    if (ImGui::ColorEdit4("終了色 1", &end1.x))
                        color.endColor = Math::ColorVectorToUint32(end1);

                    // グラデーション2
                    if (color.mode == ColorOverLifetimeModule::Mode::RandomBetweenTwo)
                    {
                        Vector4 start2 = Math::Uint32ToColorVector(color.startColor2);
                        if (ImGui::ColorEdit4("開始色 2", &start2.x))
                            color.startColor2 = Math::ColorVectorToUint32(start2);

                        Vector4 end2 = Math::Uint32ToColorVector(color.endColor2);
                        if (ImGui::ColorEdit4("終了色 2", &end2.x))
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

                    ImGui::Checkbox("有効##Size", &size.enabled);
                    ImGui::DragFloat3("開始サイズ", &size.startScale.x, 0.01f);
                    ImGui::DragFloat3("終了サイズ", &size.endScale.x, 0.01f);

                    ImGui::Separator();
                    ImGui::Checkbox("振動", &size.oscillate);
                    if (size.oscillate)
                        ImGui::DragFloat("周波数", &size.frequency, 0.1f, 0.0f, 100.0f);

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 渦モジュール
                if (ImGui::TreeNode("渦モジュール"))
                {
                    auto& vortex = config.vortex;

                    ImGui::Checkbox("有効##Vortex", &vortex.enabled);
                    ImGui::DragFloat3("中心", &vortex.center.x, 0.1f);
                    ImGui::DragFloat3("回転軸 (Axis)", &vortex.axis.x, 0.1f);
                    ImGui::DragFloat("周回スピード", &vortex.orbitalSpeed, 0.1f, -1000.0f, 1000.0f);
                    ImGui::DragFloat("半径方向スピード", &vortex.radialSpeed, 0.1f, -100.0f, 100.0f);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("中心方向への力\n負値で外向き");

                    ImGui::TreePop();
                }

                ImGui::Separator();

                // 引力モジュール
                if (ImGui::TreeNode("引力モジュール"))
                {
                    auto& attraction = config.attraction;

                    ImGui::Checkbox("有効##Attraction", &attraction.enabled);

                    if (attraction.enabled)
                    {
                        ImGui::DragFloat("強さ", &attraction.strength, 0.1f, -1000.0f, 1000.0f);

                        ImGui::Separator();

                        ImGui::Text("静的ターゲット (ターゲット未設定時)");
                        ImGui::DragFloat3("座標##AttractTarget", &attraction.target.x, 0.1f);

                        ImGui::Separator(); 

                        ImGui::Text("動的ターゲット (SetAttractionTarget使用時)");
                        ImGui::DragFloat3("オフセット##AttractOffset", &attraction.offset.x, 0.1f);
                    }

                    ImGui::TreePop();
                }
                ImGui::Separator();
                if (ImGui::TreeNode("トレイルモジュール"))
                {
                    auto& trail = config.trail; 

                    ImGui::Checkbox("有効##Trail", &trail.enabled);

                    if (trail.enabled)
                    {
                        ImGui::DragFloat("太さ", &trail.width, 0.1f, 0.1f, 10.0f);
                        ImGui::DragFloat("寿命", &trail.lifetime, 0.1f, 0.1f, 5.0f);
                        ImGui::DragFloat("最小頂点距離", &trail.minVertexDistance, 0.01f, 0.01f, 10.0f);

                        ImGui::Separator();

                        
                        // テクスチャ一覧取得
                        const auto& allDefinitions = ParticleTextureHandle::GetDefinitions();
                        std::vector<const char*> textureNameArray;
                        std::vector<ParticleTextureID> idArray;

                        for (const auto& def : allDefinitions)
                        {
                            const char* path = def.path;
                            const char* filename = path;

                            const char* lastSlash = strrchr(path, '/');
                            const char* lastBackslash = strrchr(path, '\\');
                            const char* separator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

                            if (separator)
                                filename = separator + 1;

                            textureNameArray.push_back(filename);
                            idArray.push_back(def.id);
                        }

                        int selectedTextureIdx = 0;
                        for (size_t i = 0; i < idArray.size(); ++i)
                        {
                            if (ParticleTextureHandle::Get(idArray[i]) == trail.textureHandle)
                            {
                                selectedTextureIdx = static_cast<int>(i);
                                break;
                            }
                        }

                        if (ImGui::Combo("テクスチャ", &selectedTextureIdx,
                            textureNameArray.data(),
                            (int)textureNameArray.size()))
                        {
                            trail.textureHandle = ParticleTextureHandle::Get(idArray[selectedTextureIdx]);
                        }
                        ImGui::Separator();

                        const char* modes[] = { "Stretch (全体)", "Tile (繰り返し)" };
                        int currentMode = static_cast<int>(trail.textureMode);
                        if (ImGui::Combo("テクスチャモード", &currentMode, modes, IM_ARRAYSIZE(modes)))
                        {
                            trail.textureMode = static_cast<TrailTextureMode>(currentMode);
                        }

                        ImGui::DragFloat2("タイリング (回数)", &trail.tiling.x, 0.1f);
                        ImGui::DragFloat2("スクロール速度", &trail.scrollSpeed.x, 0.01f);

                        ImGui::Separator();

                        const char* jitterModes[] = { "Wave (滑らか)", "Step (四角)", "Random (稲妻)" };
                        int currentJitter = static_cast<int>(trail.jitterMode);
                        if (ImGui::Combo("揺れタイプ", &currentJitter, jitterModes, IM_ARRAYSIZE(jitterModes)))
                        {
                            trail.jitterMode = static_cast<JitterMode>(currentJitter);
                        }

                        ImGui::Text("ジッター (形状変形)");
                        ImGui::DragFloat("強さ", &trail.jitterStrength, 0.1f, 0.0f, 50.0f);
                        ImGui::DragFloat("周波数", &trail.jitterFrequency, 0.1f, 0.1f, 100.0f);
                        ImGui::DragFloat("変動速度", &trail.jitterSpeed, 0.1f, 0.0f, 100.0f);
                        ImGui::DragFloat("位相 (位置ずらし)", &trail.jitterPhase, 0.01f, -10.0f, 10.0f);

                        ImGui::Separator();

                        ImGui::Text("ディゾルブ (侵食消滅)");

                        int selectedDissolveIdx = -1; 
                        for (size_t i = 0; i < idArray.size(); ++i)
                        {
                            if (ParticleTextureHandle::Get(idArray[i]) == trail.dissolveTextureHandle)
                            {
                                selectedDissolveIdx = static_cast<int>(i);
                                break;
                            }
                        }
                        ImGui::Separator();

                        if (ImGui::Combo("ノイズ画像", &selectedDissolveIdx, textureNameArray.data(), (int)textureNameArray.size()))
                        {
                            trail.dissolveTextureHandle = ParticleTextureHandle::Get(idArray[selectedDissolveIdx]);
                        }
                        if (ImGui::Button("ノイズ解除"))
                        {
                            trail.dissolveTextureHandle = 0;
                        }


                        ImGui::Text("太さの変化 (Width over Trail)");
                        ImGui::DragFloat("先端スケール (Head)", &trail.headWidthScale, 0.01f, 0.0f, 5.0f);
                        ImGui::DragFloat("末尾スケール (Tail)", &trail.tailWidthScale, 0.01f, 0.0f, 5.0f);

                        ImGui::Separator();

                        const char* alignModes[] = { "View (カメラ向き)", "Transform (回転追従)" };
                        int currentAlign = static_cast<int>(trail.alignment);
                        if (ImGui::Combo("向きの制御", &currentAlign, alignModes, IM_ARRAYSIZE(alignModes)))
                        {
                            trail.alignment = static_cast<TrailAlignment>(currentAlign);
                        }

                        ImGui::Separator();

                        ImGui::Text("カラー推移");
                        ImGui::ColorEdit4("開始色 (根元)", &trail.startColor.x);
                        ImGui::ColorEdit4("終了色 (先端)", &trail.endColor.x);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("ノイズモジュール"))
                {
                    auto& noise = config.noise;

                    ImGui::Checkbox("有効##Noise", &noise.enabled);
                    if (noise.enabled)
                    {
                        ImGui::DragFloat("強度", &noise.strength, 0.1f, 0.0f, 100.0f);
                        ImGui::DragFloat("周波数", &noise.frequency, 0.01f, 0.01f, 10.0f);
                        ImGui::DragFloat("スクロール速度", &noise.scrollSpeed, 0.01f, 0.0f, 10.0f);

                        ImGui::Checkbox("XYZ軸で分離", &noise.separateAxes);
       
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::Separator();

            // 保存ボタン
            if (ImGui::Button("セーブ"))
            {
                particleSystem_->configManager_->SaveParticleDefinitionToJson(selectedPresetName);

                std::string message = std::format("{}.json saved", selectedPresetName);
                // タイマーをセットする
                saveMessageTimer_ = 3.0f;
            }

            if (saveMessageTimer_ > 0.0f)
            {
                ImGui::SameLine();

                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s.json を保存", selectedPresetName.c_str());

                // 経過時間を減算
                saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
            }

            ImGui::SameLine();
        }
    }

    ImGui::End();
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