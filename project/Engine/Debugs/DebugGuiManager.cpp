#include "DebugGuiManager.h"
#include "Engine.h"
#include "TimeManager.h"
#include "ImGuiManager.h"

void DebugGuiManager::Initialize(Engine* engine, Camera* camera, LightManager* lightManager, MaterialManager* materialManager,
    TextureManager* textureManager, PostEffectManager* postEffectManager, DebugCamera* debugCamera)
{
    engine_ = engine;
    camera_ = camera;
    lightManager_ = lightManager;
    materialManager_ = materialManager;
    textureManager_ = textureManager;
    postEffectManager_ = postEffectManager;
	debugCamera_ = debugCamera;

    cameraFov_ = camera_->GetFov();
    cameraNearClip_ = camera_->GetNearClip();
    cameraFarClip_ = camera_->GetFarClip();
}

void DebugGuiManager::Update()
{
    // メインのデバッグウィンドウ
    ImGui::Begin("全体のデバッグ情報・設定");

    if (ImGui::CollapsingHeader("描画系設定"))
    {
        DrawRenderSettings();
    }
    if (ImGui::CollapsingHeader("カメラ設定"))
    {
        DrawCameraSettings();
    }
    if (ImGui::CollapsingHeader("ライト設定"))
    {
        DrawLightSettings();
    }
    if (ImGui::CollapsingHeader("ポストエフェクト設定"))
    {
        DrawPostEffectSettings();
    }
    if (ImGui::CollapsingHeader("時間 / FPS"))
    {
        DrawTimeSettings(); 
    }
    if (ImGui::CollapsingHeader("全体的な情報"))
    {
        DrawInformationDisplays();
    }

    ImGui::End();
}

void DebugGuiManager::DrawRenderSettings()
{
    ImGui::Checkbox("ワイヤーフレーム描画", &engine_->renderer_->isWireFrame_);
}

void DebugGuiManager::DrawCameraSettings()
{
    bool enabled = engine_->debugCamera_->IsEnabled();
    if (ImGui::Checkbox("デバッグカメラを有効化", &enabled))
    {
        engine_->debugCamera_->SetEnabled(enabled);
    }

    if (ImGui::TreeNode("メインカメラ"))
    {
        // 位置
        Vector3 translation = camera_->GetTranslation();
        if (ImGui::DragFloat3("座標 (World)", &translation.x, 0.1f)) {
            camera_->SetTranslation(translation);
            camera_->UpdateViewMatrix();
        }

        // 回転
        Vector3 rotationEuler = camera_->GetWorldRotationEuler();
        if (ImGui::DragFloat3("回転 (World)", &rotationEuler.x, 0.1f)) {
            camera_->SetWorldRotationEuler(rotationEuler);
            camera_->UpdateViewMatrix();
        }

        // スライダーで調整
        if (ImGui::DragFloat("視野角 (FOV)", &cameraFov_, 0.1f, 1.0f, 179.0f)) {
            camera_->SetFov(cameraFov_);
        }
        if (ImGui::DragFloat("ニアクリップ", &cameraNearClip_, 0.01f, 0.001f, 100.0f)) {
            camera_->SetNearClip(cameraNearClip_);
        }
        if (ImGui::DragFloat("ファークリップ", &cameraFarClip_, 1.0f, 1.0f, 10000.0f)) {
            camera_->SetFarClip(cameraFarClip_);
        }
        ImGui::TreePop();
    }


    // DebugCameraの内部パラメータを操作できるようにする
    if (ImGui::TreeNode("デバッグカメラ"))
    {
        // 注視点の編集
        Vector3 target = debugCamera_->GetTarget();
        if (ImGui::DragFloat3("注視点", &target.x, 0.1f)) {
            debugCamera_->SetTarget(target);
            camera_->UpdateViewMatrix();
        }

        float distance = debugCamera_->GetDistance();
        if (ImGui::DragFloat("注視点からの距離", &distance, 0.1f, 1.0f, 500.0f)) {
            debugCamera_->SetDistance(distance);
            camera_->UpdateViewMatrix();
        }

        float pitch = debugCamera_->GetCurrentPitch();
        if (ImGui::DragFloat("ピッチ (縦回転)", &pitch, 0.1f, -89.0f, 89.0f)) {
            debugCamera_->SetCurrentPitch(pitch);
            camera_->UpdateViewMatrix();
        }

        float yaw = debugCamera_->GetCurrentYaw();
        if (ImGui::DragFloat("ヨー (横回転)", &yaw, 0.1f, -180.0f, 180.0f)) {
            debugCamera_->SetCurrentYaw(yaw);
            camera_->UpdateViewMatrix();
        }

        // その他の設定の調整
        float dragSpeed = debugCamera_->GetDragSpeed();
        if (ImGui::DragFloat("ドラッグ速度 (中クリック)", &dragSpeed, 0.001f, 0.001f, 1.0f)) {
            debugCamera_->SetDragSpeed(dragSpeed);
        }

        float rotateSpeed = debugCamera_->GetRotateSpeed();
        if (ImGui::DragFloat("回転速度 (右クリック)", &rotateSpeed, 0.0001f, 0.0001f, 0.05f)) {
            debugCamera_->SetRotateSpeed(rotateSpeed);
        }

        float zoomSpeed = debugCamera_->GetZoomSpeed();
        if (ImGui::DragFloat("ズーム速度 (ホイール)", &zoomSpeed, 0.001f, 0.01f, 1.0f)) {
            debugCamera_->SetZoomSpeed(zoomSpeed);
        }
        ImGui::TreePop();
    }

    // カメラの更新を反映
    camera_->UpdateViewProjectionMatrix();
}

void DebugGuiManager::DrawLightSettings()
{
    DirectionalLight* dirLights = lightManager_->GetDirectionalLightData();
    PointLight* pointLights = lightManager_->GetPointLightData();
    SpotLight* spotLights = lightManager_->GetSpotLightData();
    AreaLight* areaLights = lightManager_->GetAreaLightData();

    MaterialSettings& materialSettings = materialManager_->GetMaterialSettings();

    ImGui::Checkbox("ライティング有効", &materialSettings.enableLighting);
    ImGui::Separator();
    // ディレクショナルライト
    if (ImGui::TreeNode("ディレクショナルライト (平行光源)"))
    {
        ImGui::Combo("ライトモード", &materialSettings.lightMode,
            "ハーフランバート\0スペキュラ\0トゥーン\0");

        for (int i = 0; i < lightManager_->GetDirectionalLightCount(); ++i)
        {
            std::string label = "ディレクショナルライト " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                bool enabled = (dirLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled))
                {
                    dirLights[i].enable = enabled ? 1 : 0;
                }
                ImGui::DragFloat3("向き", &dirLights[i].direction.x, 0.05f);
                ImGui::ColorEdit4("色", &dirLights[i].color.x);
                ImGui::DragFloat("強度", &dirLights[i].intensity, 0.01f, 0.0f, 100.0f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    // ポイントライト
    if (ImGui::TreeNode("ポイントライト (点光源)"))
    {
        for (int i = 0; i < lightManager_->GetPointLightCount(); ++i)
        {
            std::string label = "ポイントライト " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                bool enabled = (pointLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled))
                {
                    pointLights[i].enable = enabled ? 1 : 0;
                }
                ImGui::DragFloat3("座標", &pointLights[i].position.x, 0.05f);
                ImGui::ColorEdit4("色", &pointLights[i].color.x);
                ImGui::DragFloat("強度", &pointLights[i].intensity, 0.01f);
                ImGui::DragFloat("影響半径", &pointLights[i].radius, 0.1f);
                ImGui::DragFloat("減衰", &pointLights[i].decay, 0.01f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    // スポットライト
    if (ImGui::TreeNode("スポットライト"))
    {
        for (int i = 0; i < lightManager_->GetSpotLightCount(); ++i)
        {
            std::string label = "スポットライト " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                bool enabled = (spotLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled))
                {
                    spotLights[i].enable = enabled ? 1 : 0;
                }
                ImGui::DragFloat3("座標", &spotLights[i].position.x, 0.05f);
                ImGui::ColorEdit4("色", &spotLights[i].color.x);
                ImGui::DragFloat("強度", &spotLights[i].intensity, 0.01f);
                ImGui::DragFloat3("向き", &spotLights[i].direction.x, 0.05f);
                ImGui::DragFloat("距離", &spotLights[i].distance, 0.1f);
                ImGui::DragFloat("減衰", &spotLights[i].decay, 0.01f);
                ImGui::DragFloat("照射角(コサイン値)", &spotLights[i].cosAngle, 0.01f, 0.0f, 1.0f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("エリアライト (矩形光源)"))
    {
        for (int i = 0; i < lightManager_->GetAreaLightCount(); ++i)
        {
            std::string label = "エリアライト " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                bool enabled = (areaLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled))
                {
                    areaLights[i].enable = enabled ? 1 : 0;
                }
                ImGui::DragFloat3("座標 (中心)", &areaLights[i].position.x, 0.05f);
                ImGui::ColorEdit4("色", &areaLights[i].color.x);
                ImGui::DragFloat("強度", &areaLights[i].intensity, 0.01f);

                ImGui::DragFloat3("右ベクトル (幅/2)", &areaLights[i].right.x, 0.05f);
                ImGui::DragFloat3("上ベクトル (高さ/2)", &areaLights[i].up.x, 0.05f);

                ImGui::DragFloat("影響半径", &areaLights[i].range, 0.1f);
                ImGui::DragFloat("減衰", &areaLights[i].decay, 0.01f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode("影の設定"))
    {
        ImGui::Checkbox("影を受ける", &materialSettings.addShadow);
        ImGui::DragFloat("シャドウバイアス ", &materialSettings.shadowBias, 0.00001f, 0.0f, 0.01f, "%.5f");
        ImGui::SliderFloat("影の濃さ", &materialSettings.shadowDensity, 0.0f, 1.0f);
        ImGui::SliderFloat("影の柔らかさ", &materialSettings.shadowSoftness, 1.0f, 10.0f);
        ImGui::TreePop();
    }

    ImGui::Separator();

    // マテリアル設定（スペキュラ）
    if (ImGui::TreeNode("マテリアル基本設定"))
    {
        ImGui::DragFloat("拡散反射の減衰", &materialSettings.diffuseReflection, 0.1f, 1.0f, 10.0f);
        ImGui::DragFloat("光沢度 (Shininess)", &materialSettings.shininess, 1.0f, 0.0f, 256.0f);
        ImGui::ColorEdit4("鏡面反射色 (Specular)", &materialSettings.specularColor.x, 0);
        ImGui::DragFloat("環境マップの強さ", &materialSettings.environmentMapIntensity, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("エミッシブ (自己発光):発光強度", &materialSettings.emissiveIntensity, 0.1f, 0.0f, 50.0f);
        ImGui::TreePop();
    }
    ImGui::Separator();
    if (ImGui::TreeNode("リムライト"))
    {
        ImGui::Checkbox("リムライト有効", &materialSettings.enableRim);
        ImGui::Checkbox("ライト方向の影響を受ける", &materialSettings.rimUseLightDir);

        ImGui::DragFloat("鋭さ", &materialSettings.rimPower, 0.1f, 0.1f, 20.0f);

        ImGui::DragFloat("強さ", &materialSettings.rimIntensity, 0.01f, 0.0f, 10.0f);

        ImGui::ColorEdit3("発光色", &materialSettings.rimColor.x);

        ImGui::TreePop();
    }
    ImGui::Separator();
}

void DebugGuiManager::DrawPostEffectSettings()
{
    // 各データへのポインタ取得
    PostEffectData* postEffectData = postEffectManager_->GetPostEffectData();
    BrightExtractSettings* brightExtractData = postEffectManager_->GetBrightSettings();
    BlurSettings* hSettings = postEffectManager_->GetHorizontalBlurSettings();
    BlurSettings* vSettings = postEffectManager_->GetVerticalBlurSettings();
    CombineSettings* combineSettingsData = postEffectManager_->GetCombineSettings();
    GodRaySettings* godRaySettings = postEffectManager_->GetGodRaySettings();

    // カラー・色調系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "カラー・色調");

    // グレースケール
    if (ImGui::CheckboxFlags("グレースケール", &postEffectData->modeFlags[0], GRAYSCALE)) {}
    if (postEffectData->modeFlags[0] & GRAYSCALE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("適用量##Gray", &postEffectData->grayscaleColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // セピア
    if (ImGui::CheckboxFlags("セピア", &postEffectData->modeFlags[0], SEPIA)) {}
    if (postEffectData->modeFlags[0] & SEPIA)
    {
        ImGui::Indent();
        ImGui::SliderFloat("適用量##Sepia", &postEffectData->sepiaColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // カラーティント (色合い調整)
    if (ImGui::CheckboxFlags("カラーティント", &postEffectData->modeFlags[0], COLOR_TINT)) {}
    if (postEffectData->modeFlags[0] & COLOR_TINT)
    {
        ImGui::Indent();
        ImGui::ColorEdit3("着色カラー", &postEffectData->tintColor.x);
        ImGui::SliderFloat("乗算合成率", &postEffectData->tintMulColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("加算合成率", &postEffectData->tintAddColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("スクリーン合成率", &postEffectData->tintScreenColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // ヴィネット
    if (ImGui::CheckboxFlags("ビネット", &postEffectData->modeFlags[0], VIGNETTE)) {}
    if (postEffectData->modeFlags[0] & VIGNETTE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("強度", &postEffectData->vignetteAmount, 0.0f, 10.0f);
        ImGui::SliderFloat("半径", &postEffectData->vignetteRadius, 0.0f, 1.0f);
        ImGui::SliderFloat("ぼかし具合", &postEffectData->vignetteSoftness, 0.0f, 1.0f);
        ImGui::SliderFloat2("楕円スケール", &postEffectData->vignetteEllipseScale.x, 0.0f, 2.0f);
        ImGui::ColorEdit3("色", &postEffectData->vignetteColor.x);
        ImGui::Unindent();
    }

    // 色収差
    if (ImGui::CheckboxFlags("色収差", &postEffectData->modeFlags[0], CHROM_ABERRATION)) {}
    if (postEffectData->modeFlags[0] & CHROM_ABERRATION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ズレ幅", &postEffectData->chromaOffset, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // RGBずらし
    if (ImGui::CheckboxFlags("RGBずらし", &postEffectData->modeFlags[0], RGB_SPLIT)) {}
    if (postEffectData->modeFlags[0] & RGB_SPLIT)
    {
        ImGui::Indent();
        ImGui::SliderFloat("オフセット量", &postEffectData->rgbSplitOffset, 0.0f, 0.05f);
        ImGui::Unindent();
    }

    ImGui::Separator();

    // 形状・歪み系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "形状・歪み");

    // ドット化
    if (ImGui::CheckboxFlags("ドット化", &postEffectData->modeFlags[0], PIXELATION)) {}
    if (postEffectData->modeFlags[0] & PIXELATION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ドットサイズ", &postEffectData->pixelationSize, 1.0f, 64.0f);
        ImGui::Unindent();
    }

    // 画面の波紋
    if (ImGui::CheckboxFlags("画面の波紋 (Wave)", &postEffectData->modeFlags[0], SCREEN_WAVE)) {}
    if (postEffectData->modeFlags[0] & SCREEN_WAVE)
    {
        ImGui::Indent();
        const char* waveDirOptions[] = { "水平のみ", "垂直のみ", "両方向" };
        ImGui::Combo("波の方向", &postEffectData->waveDirection, waveDirOptions, IM_ARRAYSIZE(waveDirOptions));
        ImGui::SliderFloat("周波数 (密度)", &postEffectData->waveFrequency, 1.0f, 100.0f);
        ImGui::SliderFloat("振幅 (揺れ幅)", &postEffectData->waveAmplitude, 0.0f, 0.05f);
        ImGui::SliderFloat("速度", &postEffectData->waveSpeed, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // 魚眼レンズ
    if (ImGui::CheckboxFlags("魚眼レンズ", &postEffectData->modeFlags[0], FISHEYE)) {}
    if (postEffectData->modeFlags[0] & FISHEYE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("歪み強度", &postEffectData->fisheyeDistortion, 0.0f, 2.0f);
        ImGui::Unindent();
    }

    // ヒートヘイズ
    if (ImGui::CheckboxFlags("ヒートヘイズ (陽炎)", &postEffectData->modeFlags[0], HEAT_HAZE)) {}
    if (postEffectData->modeFlags[0] & HEAT_HAZE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("歪み強度", &postEffectData->heatDistortionStrength, 0.0f, 0.05f);
        ImGui::SliderFloat("ノイズスケール", &postEffectData->heatNoiseScale, 1.0f, 100.0f);
        ImGui::SliderFloat("ゆらぎ速度", &postEffectData->heatSpeed, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // 水面屈折
    if (ImGui::CheckboxFlags("水面屈折", &postEffectData->modeFlags[0], WATER_REFRACTION)) {}
    if (postEffectData->modeFlags[0] & WATER_REFRACTION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("乱流強度", &postEffectData->turbulentStrength, 0.0f, 0.1f);
        ImGui::SliderFloat("乱流周波数", &postEffectData->turbulentFrequency, 1.0f, 50.0f);
        ImGui::SliderFloat("速度", &postEffectData->turbulentSpeed, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    ImGui::Separator();

    // 特殊効果・ノイズ系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "特殊効果・ノイズ");

    // 走査線
    if (ImGui::CheckboxFlags("走査線 (Scanline)", &postEffectData->modeFlags[0], SCANLINE)) {}
    if (postEffectData->modeFlags[0] & SCANLINE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("強度", &postEffectData->scanlineIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("線の密度", &postEffectData->scanlineFrequency, 1.0f, 1000.0f);
        ImGui::SliderFloat("スクロール速度", &postEffectData->scanlineScrollSpeed, -15.0f, 15.0f);
        ImGui::ColorEdit3("線の色", &postEffectData->scanlineColor.x);
        const char* directions[] = { "水平", "垂直", "斜め" };
        ImGui::Combo("方向", &postEffectData->scanlineDirection, directions, IM_ARRAYSIZE(directions));
        ImGui::Unindent();
    }

    // スクリーンノイズ
    if (ImGui::CheckboxFlags("スクリーンノイズ (砂嵐)", &postEffectData->modeFlags[0], SCREEN_NOISE)) {}
    if (postEffectData->modeFlags[0] & SCREEN_NOISE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ノイズ量", &postEffectData->noiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("変化速度", &postEffectData->noiseSpeed, 0.0f, 10.0f);
        ImGui::SliderFloat("粒度スケール", &postEffectData->noiseScale, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // ブロックノイズ
    if (ImGui::CheckboxFlags("ブロックノイズ", &postEffectData->modeFlags[0], BLOCK_NOISE)) {}
    if (postEffectData->modeFlags[0] & BLOCK_NOISE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ノイズ量", &postEffectData->blockNoiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("ブロックサイズ", &postEffectData->blockNoiseSize, 4.0f, 128.0f);
        ImGui::SliderFloat("変化速度", &postEffectData->blockNoiseSpeed, 0.0f, 100.0f);
        ImGui::Unindent();
    }

    // フィルムグレイン
    if (ImGui::CheckboxFlags("フィルムグレイン (粒子)", &postEffectData->modeFlags[0], FILM_GRAIN)) {}
    if (postEffectData->modeFlags[0] & FILM_GRAIN)
    {
        ImGui::Indent();
        ImGui::SliderFloat("粒子強度", &postEffectData->filmGrainIntensity, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // グリッチ
    if (ImGui::CheckboxFlags("グリッチエフェクト", &postEffectData->modeFlags[0], GLITCH)) {}
    if (postEffectData->modeFlags[0] & GLITCH)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ブロックの高さ", &postEffectData->glitchBlockHeight, 0.01f, 0.2f);
        ImGui::SliderFloat("グリッチ量", &postEffectData->glitchAmount, 0.0f, 0.3f);
        ImGui::SliderFloat("ノイズ強度", &postEffectData->glitchNoiseIntensity, 0.0f, 0.5f);
        ImGui::Unindent();
    }

    // ディゾルブ
    if (ImGui::CheckboxFlags("ディゾルブ", &postEffectData->modeFlags[0], DISSOLVE)) {}
    if (postEffectData->modeFlags[0] & DISSOLVE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("進行度", &postEffectData->dissolveThreshold, 0.0f, 1.0f);
        ImGui::SliderFloat("境界の幅", &postEffectData->dissolveEdgeWidth, 0.0f, 0.2f);
        ImGui::DragFloat("発光強度", &postEffectData->dissolveEdgeIntensity, 0.1f, 0.0f, 50.0f);
        ImGui::ColorEdit3("境界色", &postEffectData->dissolveEdgeColor.x);
        ImGui::Unindent();
    }

    // 環境・光・深度設定
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "環境・光・深度設定");

    // ブルーム設定
    if (ImGui::TreeNode("ブルーム"))
    {
        if (ImGui::TreeNode("輝度抽出 (Threshold)"))
        {
            ImGui::SliderFloat("しきい値", &brightExtractData->threshold, 0.0f, 10.0f);
            ImGui::SliderFloat("抽出強度", &brightExtractData->intensity, 0.0f, 5.0f);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("ぼかし (Blur)"))
        {
            ImGui::TextDisabled("サンプリングサイズ");
            ImGui::SliderFloat("横 (X)", &hSettings->texelSize.x, 0.0f, 0.01f, "%.5f");
            ImGui::SliderFloat("縦 (Y)", &vSettings->texelSize.y, 0.0f, 0.01f, "%.5f");

            ImGui::Separator();
            ImGui::TextDisabled("ブラー強度");
            if (ImGui::SliderFloat("強さ", &hSettings->blurStrength, 0.0f, 10.0f))
                vSettings->blurStrength = hSettings->blurStrength;

            ImGui::TreePop();
        }

        ImGui::Text("ブルーム合成強度");
        ImGui::SliderFloat("Intensity", &combineSettingsData->bloomIntensity, 0.0f, 5.0f);

        ImGui::TreePop();
    }

    // ゴッドレイ設定
    if (ImGui::TreeNode("ゴッドレイ"))
    {
        ImGui::TextDisabled("合成設定");
        ImGui::SliderFloat("最終強度", &combineSettingsData->godRayIntensity, 0.0f, 5.0f);

        if (godRaySettings)
        {
            ImGui::Separator();
            ImGui::TextDisabled("生成パラメータ");
            ImGui::SliderFloat("輝度しきい値", &godRaySettings->threshold, 0.0f, 1.0f);
            ImGui::SliderFloat("密度", &godRaySettings->density, 0.0f, 2.0f);
            ImGui::DragFloat("減衰率", &godRaySettings->decay, 0.001f, 0.8f, 0.999f, "%.4f");
            ImGui::SliderFloat("重み", &godRaySettings->weight, 0.0f, 1.0f);
            ImGui::SliderFloat("露出", &godRaySettings->exposure, 0.0f, 5.0f);

            int samples = godRaySettings->numSamples;
            if (ImGui::SliderInt("サンプル数", &samples, 16, 128)) {
                godRaySettings->numSamples = samples;
            }
        }
        ImGui::TreePop();
    }

    // 被写界深度 (DoF)
    if (ImGui::TreeNode("被写界深度 (DoF)"))
    {
        bool dofFlag = (combineSettingsData->enableDoF != 0);
        if (ImGui::Checkbox("DoF有効", &dofFlag))
            combineSettingsData->enableDoF = dofFlag ? 1 : 0;

        if (dofFlag)
        {
            ImGui::Indent();
            ImGui::SliderFloat("ピント距離", &combineSettingsData->focusDistance, 0.1f, 500.0f, "%.1f m");
            ImGui::SliderFloat("ピント範囲", &combineSettingsData->focusRange, 0.1f, 500.0f, "%.1f m");
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    // フォグ設定
    if (ImGui::TreeNode("フォグ"))
    {
        bool fogFlag = (combineSettingsData->enableFog != 0);
        if (ImGui::Checkbox("フォグ有効", &fogFlag))
            combineSettingsData->enableFog = fogFlag ? 1 : 0;

        if (fogFlag)
        {
            ImGui::Indent();
            ImGui::ColorEdit3("フォグ色", &combineSettingsData->fogColor.x);
            ImGui::DragFloat("開始距離", &combineSettingsData->fogStart, 0.1f, 0.0f, 500.0f, "%.1f m");
            ImGui::DragFloat("終了距離", &combineSettingsData->fogEnd, 0.1f, 0.0f, 1000.0f, "%.1f m");

            if (combineSettingsData->fogStart > combineSettingsData->fogEnd)
                combineSettingsData->fogStart = combineSettingsData->fogEnd;
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }
    

    ImGui::Separator();
}

void DebugGuiManager::DrawTimeSettings()
{
    TimeManager* time = TimeManager::GetInstance();

    // 時間の制御

    // 一時停止
    bool isPaused = time->IsPaused();
    if (ImGui::Checkbox("一時停止", &isPaused))
    {
        if (isPaused) {
            time->Pause();
        }
        else {
            time->Resume();
        }
    }

    // タイムスケール (スローモーション/早送りデバッグ)
    float timeScale = time->GetTimeScale();
    if (ImGui::DragFloat("タイムスケール", &timeScale, 0.01f, 0.0f, 10.0f))
    {
        time->SetTimeScale(timeScale);
    }

    ImGui::SameLine();
    if (ImGui::Button("リセット"))
    {
        time->SetTimeScale(1.0f);
    }


    ImGui::Separator(); // 制御と表示を分離

    // 時間情報の表示

    // FPS関連
    ImGui::Text("平均 FPS: %.1f", time->GetAverageFPS()); 
    ImGui::Text("瞬間 FPS: %.1f", time->GetFPS());

    // DeltaTime
    ImGui::Text("DeltaTime (Scaled):   %.2f ms", time->GetDeltaTime() * 1000.0f);
    ImGui::Text("DeltaTime (Unscaled): %.2f ms", time->GetUnscaledDeltaTime() * 1000.0f);

    // 実行時間
    ImGui::Text("総実行時間 (TotalTime): %.2f s", time->GetTotalTime());
}

void DebugGuiManager::DrawInformationDisplays() 
{
    // オブジェクト数
    ImGui::Text("Models: %d / %d", engine_->renderer_->GetModelCount(), engine_->renderer_->kMaxModelCount);
    ImGui::Text("Sprites: %d / %d", engine_->renderer_->GetSpriteCount(), engine_->renderer_->kMaxSpriteCount);
    ImGui::Text("Lines: %d / %d", engine_->renderer_->GetLineCount(), engine_->renderer_->kMaxLineCount);
    ImGui::Text("Particles: %d / %d", engine_->renderer_->GetParticleCount(), engine_->renderer_->kMaxParticleCount);
    ImGui::Text("Trails: %d / %d", engine_->renderer_->GetTrailCount(), engine_->renderer_->kMaxTrailCount);
}

void DebugGuiManager::BeginSceneView(
    SRVManager* srvManager,
    uint32_t srvIndexToShow
) 
{
    /// GPUハンドル取得
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvManager->GetSRVHandleGPU(srvIndexToShow);

    // リセット要求などのロジック
    ImGuiCond cond = ImGuiCond_FirstUseEver;
    if (ImGuiManager::GetSceneResetRequested())
    {
        cond = ImGuiCond_Always;
        ImGuiManager::ClearSceneResetRequested();
    }

    // ウィンドウ設定
    ImGui::SetNextWindowSize(ImVec2(800, 450), cond);
    ImGui::SetNextWindowPos(ImVec2(0, 0), cond);

    // パディングなしで開始
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene");
    ImGui::PopStyleVar();

    // 画像を描画（命令の記録）
    ImVec2 imageSize = ImGui::GetContentRegionAvail();
    ImGui::Image(
        reinterpret_cast<ImTextureID>(reinterpret_cast<void*>(gpuHandle.ptr)),
        imageSize
    );

    // Gizmoの準備をここで行う
    ImVec2 vMin = ImGui::GetItemRectMin();
    ImVec2 vMax = ImGui::GetItemRectMax();
    ImGuizmo::SetRect(vMin.x, vMin.y, vMax.x - vMin.x, vMax.y - vMin.y);
    ImGuizmo::SetDrawlist();
}

void DebugGuiManager::EndSceneView()
{
    // ウィンドウを閉じるだけ
    ImGui::End();
}