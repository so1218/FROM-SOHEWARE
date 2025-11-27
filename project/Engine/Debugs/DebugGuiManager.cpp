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
    ImGui::Begin("全体のデバッグ情報");

    if (ImGui::CollapsingHeader("描画系"))
    {
        DrawRenderSettings();
    }
    if (ImGui::CollapsingHeader("カメラ"))
    {
        DrawCameraSettings();
    }
    if (ImGui::CollapsingHeader("ライト"))
    {
        DrawLightSettings();
    }
    if (ImGui::CollapsingHeader("ポストエフェクト"))
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
    ImGui::Checkbox("デバッグ描画", &engine_->useDebugView_);
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

                // right と up は、ライトの向きとサイズ（半分の幅/高さ）を制御します
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
        ImGui::TreePop();
    }

}

void DebugGuiManager::DrawPostEffectSettings()
{
    PostEffectData* postEffectData = postEffectManager_->postEffectData_;
    BrightExtractSettings* brightExtractData = postEffectManager_->brightExtractData_;
    BlurSettings* blurSettingsData = postEffectManager_->blurSettingsData_;
    CombineSettings* combineSettingsData = postEffectManager_->combineSettingsData_;

    ImGui::CheckboxFlags("None", &postEffectData->modeFlags[0], NONE);
    if (ImGui::TreeNode("PostEffectMode"))
  {
        if (ImGui::TreeNode("Mode[0]1~7"))
        {
            // 下位32bit（modeFlags[0]）
            ImGui::CheckboxFlags("Grayscale", &postEffectData->modeFlags[0], GRAYSCALE);
            ImGui::CheckboxFlags("Invert Color", &postEffectData->modeFlags[0], INVERT_COLOR);
            ImGui::CheckboxFlags("Sepia", &postEffectData->modeFlags[0], SEPIA);
            ImGui::CheckboxFlags("Brightness", &postEffectData->modeFlags[0], BRIGHTNESS);
            ImGui::CheckboxFlags("Posterization", &postEffectData->modeFlags[0], POSTERIZATION);
            ImGui::CheckboxFlags("Pixelation", &postEffectData->modeFlags[0], PIXELATION);
            ImGui::CheckboxFlags("ColorTint", &postEffectData->modeFlags[0], COLOR_TINT);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mode[0]8~16"))
        {
            // 下位32bit（modeFlags[0]）
            ImGui::CheckboxFlags("Contrast", &postEffectData->modeFlags[0], CONTRAST);
            ImGui::CheckboxFlags("Saturation", &postEffectData->modeFlags[0], SATURATION);
            ImGui::CheckboxFlags("HueShift", &postEffectData->modeFlags[0], HUE_SHIFT);
            ImGui::CheckboxFlags("ChannelSwap", &postEffectData->modeFlags[0], CHANNEL_SWAP);
            ImGui::CheckboxFlags("CelShading", &postEffectData->modeFlags[0], CEL_SHADING);
            ImGui::CheckboxFlags("NormalOutline", &postEffectData->modeFlags[0], NORMAL_OUTLINE);
            ImGui::CheckboxFlags("BrightExtract", &postEffectData->modeFlags[0], BRIGHT_EXTRACT);
            ImGui::CheckboxFlags("Vignette", &postEffectData->modeFlags[0], VIGNETTE);

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mode[0]16~24"))
        {
            ImGui::CheckboxFlags("ScreenNoise", &postEffectData->modeFlags[0], SCREEN_NOISE);
            ImGui::CheckboxFlags("ChromaticAberration", &postEffectData->modeFlags[0], CHROM_ABERRATION);
            ImGui::CheckboxFlags("ScreenWave", &postEffectData->modeFlags[0], SCREEN_WAVE);
            ImGui::CheckboxFlags("FisheyeLens", &postEffectData->modeFlags[0], FISHEYE);
            ImGui::CheckboxFlags("Flash", &postEffectData->modeFlags[0], FLASH);
            ImGui::CheckboxFlags("CRTScanline", &postEffectData->modeFlags[0], SCANLINE);
            ImGui::CheckboxFlags("BlockNoise", &postEffectData->modeFlags[0], BLOCK_NOISE);
            ImGui::CheckboxFlags("Solarize", &postEffectData->modeFlags[0], SOLARIZE);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mode[0]25~32"))
        {
            ImGui::CheckboxFlags("MultiPosterize", &postEffectData->modeFlags[0], MULTI_POSTERIZE);
            ImGui::CheckboxFlags("RGBSplitHorizontal", &postEffectData->modeFlags[0], RGB_SPLIT);
            ImGui::CheckboxFlags("InvertByY", &postEffectData->modeFlags[0], INVERT_BY_Y);
            ImGui::CheckboxFlags("FilmGrain", &postEffectData->modeFlags[0], FILM_GRAIN);
            ImGui::CheckboxFlags("Glitch", &postEffectData->modeFlags[0], GLITCH);
            ImGui::CheckboxFlags("EdgeDetection", &postEffectData->modeFlags[0], EDGE_DETECTION);
            ImGui::CheckboxFlags("HeatHaze", &postEffectData->modeFlags[0], HEAT_HAZE);
            ImGui::CheckboxFlags("SplitToning", &postEffectData->modeFlags[0], SPLIT_TONING);
            ImGui::CheckboxFlags("WaterReaction", &postEffectData->modeFlags[0], WATER_REFRACTION);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mode[1]1~16"))
        {
            // 上位32bit（modeFlags[1]）
            ImGui::CheckboxFlags("RoughEdge", &postEffectData->modeFlags[1], ROUGH_EDGE);
            ImGui::CheckboxFlags("SpiralWarp", &postEffectData->modeFlags[1], SPIRAL_WARP);
            ImGui::CheckboxFlags("RadialWave", &postEffectData->modeFlags[1], RADIAL_WAVE);
            ImGui::CheckboxFlags("GlowingOutline", &postEffectData->modeFlags[1], GLOW_OUTLINE);
            ImGui::CheckboxFlags("FBMNoise", &postEffectData->modeFlags[1], FBM_NOISE);
            ImGui::CheckboxFlags("Flare", &postEffectData->modeFlags[1], FLARE);
            ImGui::CheckboxFlags("BallEffect", &postEffectData->modeFlags[1], BALL_EFFECT);
            ImGui::CheckboxFlags("DotBlink", &postEffectData->modeFlags[1], DOT_BLINK);
            ImGui::CheckboxFlags("Outline", &postEffectData->modeFlags[1], OUTLINE);
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Neon"))
    {
        if (ImGui::TreeNode("Bright Extract Settings"))
        {
            ImGui::SliderFloat("Threshold", &brightExtractData->threshold, 0.0f, 10.0f);
            ImGui::SliderFloat("Intensity", &brightExtractData->intensity, 0.0f, 5.0f);
            ImGui::TreePop();
        }


        if (ImGui::TreeNode("Blur Settings"))
        {
            ImGui::SliderFloat2("Texel Size", &blurSettingsData->texelSize.x, 0.0f, 0.1f);
            ImGui::SliderFloat("Blur Strength", &blurSettingsData->blurStrength, 0.0f, 10.0f);
            ImGui::TreePop();
        }


        if (ImGui::TreeNode("Bloom Settings"))
        {
            ImGui::SliderFloat("Brightness Threshold", &combineSettingsData->bloomIntensity, 0.0f, 10.0f);
            static const char* modeNames[] = { "Halo", "Neon", "Bloom" };
            ImGui::Combo("Effect Mode", &combineSettingsData->effectMode, modeNames, IM_ARRAYSIZE(modeNames));
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }

    ImGui::Text("PostEffect Flags:");
    ImGui::Separator();

    if (postEffectData->modeFlags[0] & GRAYSCALE)
    {
        ImGui::SliderFloat("Grayscale Amount", &postEffectData->grayscaleColorAmount, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & SEPIA)
    {
        ImGui::SliderFloat("Sepia Amount", &postEffectData->sepiaColorAmount, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & INVERT_COLOR)
    {
        ImGui::SliderFloat("Invert Amount", &postEffectData->invertColorAmount, 0.0f, 1.0f);
    }  
    if (postEffectData->modeFlags[0] & BRIGHTNESS)
    {
        ImGui::SliderFloat("Brightness Value", &postEffectData->brightnessValue, -1.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & POSTERIZATION) 
    {
        ImGui::SliderFloat("Posterization Levels", &postEffectData->posterizationLevels, 2.0f, 32.0f);
    }
    if (postEffectData->modeFlags[0] & PIXELATION)
    {
        ImGui::SliderFloat("Pixelation Size", &postEffectData->pixelationSize, 1.0f, 64.0f);
    }
    if (postEffectData->modeFlags[0] & COLOR_TINT) 
    {
        ImGui::ColorEdit3("Tint Color", &postEffectData->tintColor.x);
        ImGui::SliderFloat("Multiply Amount", &postEffectData->tintMulColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("Additive Amount", &postEffectData->tintAddColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("Screen Amount", &postEffectData->tintScreenColorAmount, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & CONTRAST) 
    {
        ImGui::SliderFloat("Contrast Value", &postEffectData->contrastValue, 0.0f, 3.0f);
    }
    if (postEffectData->modeFlags[0] & SATURATION) 
    {
        ImGui::SliderFloat("Saturation Value", &postEffectData->saturationValue, 0.0f, 2.0f);
    }
    if (postEffectData->modeFlags[0] & HUE_SHIFT)
    {
        ImGui::SliderFloat("Hue Shift Amount", &postEffectData->hueShiftAmount, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & CHANNEL_SWAP)
    {
        const char* items[] = { "BGR", "GRB", "GBR", "BRG", "RBG" };
        ImGui::Combo("Channel Swap Mode", &postEffectData->channelSwapMode, items, IM_ARRAYSIZE(items));
    }
    if (postEffectData->modeFlags[0] & CEL_SHADING)
    {
        ImGui::SliderFloat("Cel Shading Levels", &postEffectData->celShadingLevels, 2.0f, 20.0f, "%.0f");
    }
    if (postEffectData->modeFlags[0] & NORMAL_OUTLINE)
    {
        ImGui::SliderFloat("Outline Threshold", &postEffectData->normalOutlineThreshold, 0.0f, 2.0f);
        ImGui::SliderFloat("Outline Thickness", &postEffectData->normalOutlineThickness, 0.5f, 5.0f);
        ImGui::ColorEdit3("Outline Color", &postEffectData->normalOutlineColor.x);
    }
    if (postEffectData->modeFlags[0] & VIGNETTE) 
    {
        ImGui::SliderFloat("Vignette Amount", &postEffectData->vignetteAmount, 0.0f, 10.0f);
        ImGui::SliderFloat("Vignette Radius", &postEffectData->vignetteRadius, 0.0f, 1.0f);
        ImGui::SliderFloat("Vignette Softness", &postEffectData->vignetteSoftness, 0.0f, 1.0f);
        ImGui::SliderFloat2("Vignette EllipseScale", &postEffectData->vignetteEllipseScale.x, 0.0f, 2.0f);
        ImGui::ColorEdit3("Vignette Color", &postEffectData->vignetteColor.x);
    }
    if (postEffectData->modeFlags[0] & SCREEN_NOISE)
    {
        ImGui::SliderFloat("Noise Amount", &postEffectData->noiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("Noise Speed", &postEffectData->noiseSpeed, 0.0f, 10.0f);
        ImGui::SliderFloat("Noise Scale", &postEffectData->noiseScale, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & CHROM_ABERRATION)
    {
        ImGui::SliderFloat("Chroma Offset", &postEffectData->chromaOffset, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[0] & SCREEN_WAVE)  
    {
        const char* waveDirOptions[] = { "Horizontal", "Vertical", "Both" };
        ImGui::Combo("Screen Wave Direction", &postEffectData->waveDirection, waveDirOptions, IM_ARRAYSIZE(waveDirOptions));
        ImGui::SliderFloat("Wave Frequency", &postEffectData->waveFrequency, 1.0f, 100.0f);
        ImGui::SliderFloat("Wave Amplitude", &postEffectData->waveAmplitude, 0.0f, 0.05f);
        ImGui::SliderFloat("Wave Speed", &postEffectData->waveSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[0] & FISHEYE) 
    {
        ImGui::SliderFloat("FisheyeLens", &postEffectData->fisheyeDistortion, 0.0f, 2.0f);
    }
    if (postEffectData->modeFlags[0] & FLASH)
    {
        ImGui::SliderFloat("Flash Frequency", &postEffectData->flashFrequency, 0.1f, 10.0f);
        ImGui::SliderFloat("Flash Intensity", &postEffectData->flashIntensity, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & SCANLINE)
    {
        ImGui::SliderFloat("Scanline Intensity", &postEffectData->scanlineIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Scanline Frequency", &postEffectData->scanlineFrequency, 1.0f, 1000.0f);
        ImGui::SliderFloat("Scanline Scroll Speed", &postEffectData->scanlineScrollSpeed, -15.0f, 15.0f);
        ImGui::ColorEdit3("Scanline Color", &postEffectData->scanlineColor.x);

        const char* directions[] = { "Horizontal", "Vertical", "Diagonal" };
        ImGui::Combo("Scanline Direction", &postEffectData->scanlineDirection, directions, IM_ARRAYSIZE(directions));
    }
    if (postEffectData->modeFlags[0] & BLOCK_NOISE)
    {
        ImGui::SliderFloat("Block Noise Amount", &postEffectData->blockNoiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("Block Size", &postEffectData->blockNoiseSize, 4.0f, 128.0f);
        ImGui::SliderFloat("Noise Speed", &postEffectData->blockNoiseSpeed, 0.0f, 100.0f);
    }
    if (postEffectData->modeFlags[0] & SOLARIZE)
    {
        ImGui::SliderFloat("Solarize Threshold", &postEffectData->solarizeThreshold, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & MULTI_POSTERIZE) 
    {
        ImGui::SliderFloat("Posterize Levels", &postEffectData->multiPosterizeLevels, 2.0f, 32.0f);
    }
    if (postEffectData->modeFlags[0] & RGB_SPLIT)  
    {
        ImGui::SliderFloat("RGB Split Offset", &postEffectData->rgbSplitOffset, 0.0f, 0.05f);
    }
    if (postEffectData->modeFlags[0] & FILM_GRAIN)
    {
        ImGui::SliderFloat("Film Grain Intensity", &postEffectData->filmGrainIntensity, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & GLITCH)
    {
        ImGui::SliderFloat("Glitch Block Height", &postEffectData->glitchBlockHeight, 0.01f, 0.2f);
        ImGui::SliderFloat("Glitch Amount", &postEffectData->glitchAmount, 0.0f, 0.3f);
        ImGui::SliderFloat("Glitch Noise Intensity", &postEffectData->glitchNoiseIntensity, 0.0f, 0.5f);
    }
    if (postEffectData->modeFlags[0] & EDGE_DETECTION)
    {
        ImGui::SliderFloat("Edge Threshold", &postEffectData->edgeThreshold, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & HEAT_HAZE)
    {
        ImGui::SliderFloat("Distortion Strength", &postEffectData->heatDistortionStrength, 0.0f, 0.05f);
        ImGui::SliderFloat("Noise Scale", &postEffectData->heatNoiseScale, 1.0f, 100.0f);
        ImGui::SliderFloat("Speed", &postEffectData->heatSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[0] & SPLIT_TONING)
    {
        ImGui::ColorEdit3("Shadow Color", &postEffectData->shadowColor.x);
        ImGui::ColorEdit3("Highlight Color", &postEffectData->highlightColor.x);
        ImGui::SliderFloat("Split Tone Strength", &postEffectData->splitToneStrength, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & WATER_REFRACTION)
    {
        ImGui::SliderFloat("Turbulent Strength", &postEffectData->turbulentStrength, 0.0f, 0.1f);
        ImGui::SliderFloat("Turbulent Frequency", &postEffectData->turbulentFrequency, 1.0f, 50.0f);
        ImGui::SliderFloat("Turbulent Speed", &postEffectData->turbulentSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[1] & ROUGH_EDGE) 
    {
        ImGui::SliderFloat("Edge Threshold", &postEffectData->roughEdgeThreshold, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &postEffectData->roughEdgeRoughness, 0.0f, 2.0f);
        ImGui::SliderFloat("Noise Scale", &postEffectData->roughEdgeNoiseScale, 1.0f, 100.0f);
        ImGui::SliderFloat("Speed", &postEffectData->roughEdgeSpeed, 0.0f, 10.0f);
        ImGui::ColorEdit3("Rough Edge Color", (float*)&postEffectData->roughEdgeColor);
    }
    if (postEffectData->modeFlags[1] & SPIRAL_WARP)
    {
        ImGui::SliderFloat("Base Amplitude", &postEffectData->spiralBaseAmplitude, 0.0f, 5.0f);
        ImGui::SliderFloat("Frequency", &postEffectData->spiralFrequency, 1.0f, 20.0f);
        ImGui::SliderFloat("Distance Falloff", &postEffectData->spiralDistanceFalloff, 0.1f, 5.0f);
        ImGui::SliderFloat("Noise Amount", &postEffectData->spiralNoiseAmount, 0.0f, 2.0f);

        ImGui::SliderFloat("Noise Speed", &postEffectData->spiralNoiseSpeed, 0.0f, 5.0f);
        ImGui::SliderFloat("Noise Scale", &postEffectData->spiralNoiseScale, 0.1f, 20.0f);
        ImGui::SliderFloat("Rotation Speed", &postEffectData->spiralRotationSpeed, 0.0f, 5.0f);
        ImGui::SliderFloat("Spiral Speed", &postEffectData->spiralSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[1] & RADIAL_WAVE)
    {
        ImGui::SliderFloat("Wave Amplitude", &postEffectData->radialWaveAmplitude, 0.0f, 20.0f);
        ImGui::SliderFloat("Wave Frequency", &postEffectData->radialWaveFrequency, 1.0f, 50.0f);
        ImGui::SliderFloat("Wave Speed", &postEffectData->radialWaveSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[1] & GLOW_OUTLINE) 
    {
        ImGui::SliderFloat("Outline Threshold", &postEffectData->glowOutlineThreshold, 0.0f, 2.0f);
        ImGui::SliderFloat("Outline Thickness", &postEffectData->glowOutlineThickness, 0.5f, 5.0f);
        ImGui::ColorEdit3("Outline Color", &postEffectData->glowOutlineColor.x);
        ImGui::SliderFloat("Outline Intensity", &postEffectData->glowOutlineIntensity, 0.0f, 5.0f);
    }
    if (postEffectData->modeFlags[1] & FBM_NOISE)
    {
        ImGui::SliderInt("FBM Octaves", &postEffectData->fbmOctaves, 1, 8);
        ImGui::SliderFloat("FBM Gain", &postEffectData->fbmGain, 0.1f, 1.0f);
        ImGui::SliderFloat("FBM Lacunarity", &postEffectData->fbmLacunarity, 1.0f, 4.0f);
        ImGui::SliderFloat("FBM Sharpness", &postEffectData->fbmSharpness, 0.01f, 0.5f);

        ImGui::SliderFloat("FBM Noise Intensity", &postEffectData->fbmNoiseIntensity, 0.0f, 5.0f);
        ImGui::ColorEdit3("FBM Noise Color", (float*)&postEffectData->fbmNoiseColor);
    }
    if (postEffectData->modeFlags[1] & FLARE)
    {
        ImGui::ColorEdit3("Flare Color", &postEffectData->flareColor.x);
        ImGui::SliderFloat("Flare Intensity", &postEffectData->flareIntensity, 0.0f, 5.0f);

        ImGui::SliderFloat("Flare Falloff", &postEffectData->flareFalloff, 0.1f, 10.0f);
        ImGui::SliderFloat("Ghost Distance", &postEffectData->flareGhostDistance, 0.0f, 2.0f);
        ImGui::SliderFloat("Ghost Intensity", &postEffectData->flareGhostIntensity, 0.0f, 1.0f);

        ImGui::SliderFloat("Streak Count", &postEffectData->flareStreakCount, 2.0f, 16.0f);
        ImGui::SliderFloat("Streak Speed", &postEffectData->flareStreakSpeed, -10.0f, 10.0f);
        ImGui::SliderFloat("Streak Sharpness", &postEffectData->flareStreakSharpness, 1.0f, 32.0f);
        ImGui::SliderFloat("Streak Intensity", &postEffectData->flareStreakIntensity, 0.0f, 2.0f);
    }
    if (postEffectData->modeFlags[1] & BALL_EFFECT)
    {
        ImGui::SliderFloat2("Ball Radius", &postEffectData->ballRadiusValue.x, 0.0f, 1.0f);
        ImGui::DragFloat2("Ball Position", &postEffectData->ballPosition.x, 0.01f, 0.0f, 1.0f);
        ImGui::SliderFloat("Noise Amount", &postEffectData->ballNoiseAmount, 0.0f, 0.1f);
        ImGui::SliderFloat("Time Speed", &postEffectData->ballTimeSpeed, 0.0f, 5.0f);
        ImGui::ColorEdit3("Ball Color", &postEffectData->ballColorAdjustment.x);
    }
    if (postEffectData->modeFlags[1] & DOT_BLINK)
    {
        ImGui::SliderFloat("Dot Blink Size", &postEffectData->dotBlinkSize, 0.0f, 10.0f);
        ImGui::SliderFloat("Dot Blink Speed", &postEffectData->dotBlinkSpeed, 0.0f, 10.0f);
    }
}

void DebugGuiManager::DrawTimeSettings()
{
    TimeManager* time = TimeManager::GetInstance();

    // 時間の制御

    // 一時停止
    bool isPaused = time->IsPaused();
    if (ImGui::Checkbox("一時停止 (Pause)", &isPaused))
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
    ImGui::Text("Triangles: %d / %d", engine_->renderer_->GetTriangleCount(), engine_->renderer_->kMaxTriangleCount);
    ImGui::Text("Spheres: %d / %d", engine_->renderer_->GetSphereCount(), engine_->renderer_->kMaxSphereCount);
    ImGui::Text("Models: %d / %d", engine_->renderer_->GetModelCount(), engine_->renderer_->kMaxModelCount);
    ImGui::Text("Sprites: %d / %d", engine_->renderer_->GetSpriteCount(), engine_->renderer_->kMaxSpriteCount);
    ImGui::Text("Cubes: %d / %d", engine_->renderer_->GetCubeCount(), engine_->renderer_->kMaxCubeCount);
    ImGui::Text("Lines: %d / %d", engine_->renderer_->GetLineCount(), engine_->renderer_->kMaxLineCount);
    ImGui::Text("Particles: %d / %d", engine_->renderer_->GetParticleCount(), engine_->renderer_->kMaxParticleCount);

    // プロファイリング情報 (別途プロファイリングシステムが必要)
   /* ImGui::Text("Profiling Info: [Not Implemented]");*/

    // デバッグ用テキストオーバーレイの例
    // ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 10), IM_COL32_WHITE, "Custom Overlay Text");
}

void DebugGuiManager::RenderOffscreenTexture(
    SRVManager* srvManager,
    uint32_t srvIndexToShow
) 
{
    // 1. マネージャから、表示したいSRVの「GPUハンドル」を直接もらう
     //    (コピーもCPUハンドルも不要)
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvManager->GetSRVHandleGPU(srvIndexToShow);

    // 2. ImGuiで表示する
    ImGuiCond cond = ImGuiCond_FirstUseEver; // デフォルトは初回のみ

    // リセット要求が来ているかチェック
    if (ImGuiManager::GetSceneResetRequested())
    {
        cond = ImGuiCond_Always;             // このフレームだけ強制適用
        ImGuiManager::ClearSceneResetRequested(); // フラグを下ろす
    }

    // cond 変数を使って設定
    ImGui::SetNextWindowSize(ImVec2(800, 450), cond);
    ImGui::SetNextWindowPos(ImVec2(0, 0), cond); // 必要なら位置もリセット
    ImGui::Begin("Scene");
    ImVec2 imageSize = ImGui::GetContentRegionAvail();

    // (void*) キャストは ImGui の作法なので、reinterpret_cast が2回必要
    ImGui::Image(
        reinterpret_cast<ImTextureID>(reinterpret_cast<void*>(gpuHandle.ptr)),
        imageSize
    );

    ImGui::End();
}
