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
    PostEffectData* postEffectData = postEffectManager_->GetPostEffectData();
    BrightExtractSettings* brightExtractData = postEffectManager_->GetBrightSettings();
    BlurSettings* hSettings = postEffectManager_->GetHorizontalBlurSettings();        
    BlurSettings* vSettings = postEffectManager_->GetVerticalBlurSettings(); 
    CombineSettings* combineSettingsData = postEffectManager_->GetCombineSettings();

    if (ImGui::TreeNode("ポストエフェクト設定"))
    {
        // カラー・色調系
        ImGui::TextDisabled("カラー・色調");
        ImGui::CheckboxFlags("グレースケール", &postEffectData->modeFlags[0], GRAYSCALE);
        ImGui::CheckboxFlags("セピア", &postEffectData->modeFlags[0], SEPIA);
        ImGui::CheckboxFlags("カラーティント", &postEffectData->modeFlags[0], COLOR_TINT);
        ImGui::CheckboxFlags("ビネット", &postEffectData->modeFlags[0], VIGNETTE);
        ImGui::CheckboxFlags("RGBずらし (スプリット)", &postEffectData->modeFlags[0], RGB_SPLIT);
        ImGui::CheckboxFlags("色収差", &postEffectData->modeFlags[0], CHROM_ABERRATION);

        ImGui::Separator();

        // 形状・歪み系
        ImGui::TextDisabled("形状・歪み");
        ImGui::CheckboxFlags("ドット化 (モザイク)", &postEffectData->modeFlags[0], PIXELATION);
        ImGui::CheckboxFlags("画面の波紋 (Wave)", &postEffectData->modeFlags[0], SCREEN_WAVE);
        ImGui::CheckboxFlags("魚眼レンズ", &postEffectData->modeFlags[0], FISHEYE);
        ImGui::CheckboxFlags("ヒートハイズ (陽炎)", &postEffectData->modeFlags[0], HEAT_HAZE);
        ImGui::CheckboxFlags("水面屈折", &postEffectData->modeFlags[0], WATER_REFRACTION);

        ImGui::Separator();

        // 特殊効果・ノイズ系
        ImGui::TextDisabled("特殊効果・ノイズ");
        ImGui::CheckboxFlags("走査線", &postEffectData->modeFlags[0], SCANLINE);
        ImGui::CheckboxFlags("スクリーンノイズ", &postEffectData->modeFlags[0], SCREEN_NOISE);
        ImGui::CheckboxFlags("ブロックノイズ", &postEffectData->modeFlags[0], BLOCK_NOISE);
        ImGui::CheckboxFlags("フィルムグレイン", &postEffectData->modeFlags[0], FILM_GRAIN);
        ImGui::CheckboxFlags("グリッチエフェクト", &postEffectData->modeFlags[0], GLITCH);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("ブルーム設定"))
    {
        // 輝度抽出設定
        if (ImGui::TreeNode("輝度抽出設定"))
        {
            ImGui::SliderFloat("抽出する明るさのしきい値", &brightExtractData->threshold, 0.0f, 10.0f);
            ImGui::SliderFloat("抽出時の強度", &brightExtractData->intensity, 0.0f, 5.0f);
            ImGui::TreePop();
        }

        // ぼかし設定
        if (ImGui::TreeNode("ぼかし設定 (Blur)"))
        {
            ImGui::TextDisabled("横方向");
            ImGui::SliderFloat("サイズ X", &hSettings->texelSize.x, 0.0f, 0.01f, "%.5f");

            ImGui::Separator();

            ImGui::TextDisabled("縦方向");
            ImGui::SliderFloat("サイズ Y", &vSettings->texelSize.y, 0.0f, 0.01f, "%.5f");

            ImGui::Separator();

            ImGui::TextDisabled("共通");
            if (ImGui::SliderFloat("強さ", &hSettings->blurStrength, 0.0f, 10.0f))
                vSettings->blurStrength = hSettings->blurStrength;

            ImGui::TreePop();
        }

        // 合成・DoF設定
        if (ImGui::TreeNode("合成・DoF設定"))
        {
            ImGui::TextDisabled("ブルーム");
            ImGui::SliderFloat("合成強度", &combineSettingsData->bloomIntensity, 0.0f, 5.0f);

            ImGui::Separator();

            ImGui::TextDisabled("被写界深度 (DoF)");
            bool dofFlag = (combineSettingsData->enableDoF != 0);
            if (ImGui::Checkbox("有効", &dofFlag))
                combineSettingsData->enableDoF = dofFlag ? 1 : 0;

            if (!dofFlag) ImGui::BeginDisabled();
            ImGui::SliderFloat("ピント距離", &combineSettingsData->focusDistance, 0.1f, 500.0f, "%.1f");
            ImGui::SliderFloat("ピント範囲", &combineSettingsData->focusRange, 0.1f, 500.0f, "%.1f");
            if (!dofFlag) ImGui::EndDisabled();

            ImGui::TreePop();
        }

        // フォグ設定
        if (ImGui::TreeNode("フォグ設定"))
        {
            bool fogFlag = (combineSettingsData->enableFog != 0);
            if (ImGui::Checkbox("有効", &fogFlag))
                combineSettingsData->enableFog = fogFlag ? 1 : 0;

            if (!fogFlag) ImGui::BeginDisabled();
            ImGui::ColorEdit3("色", &combineSettingsData->fogColor.x);
            ImGui::DragFloat("開始距離", &combineSettingsData->fogStart, 0.1f, 0.0f, 500.0f, "%.1f m");
            ImGui::DragFloat("終了距離", &combineSettingsData->fogEnd, 0.1f, 0.0f, 1000.0f, "%.1f m");
            if (!fogFlag) ImGui::EndDisabled();

            if (combineSettingsData->fogStart > combineSettingsData->fogEnd)
                combineSettingsData->fogStart = combineSettingsData->fogEnd;

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
    ImGui::Separator();

    if (postEffectData->modeFlags[0] & GRAYSCALE)
    {
        ImGui::SliderFloat("Grayscale Amount", &postEffectData->grayscaleColorAmount, 0.0f, 1.0f);
    }
    if (postEffectData->modeFlags[0] & SEPIA)
    {
        ImGui::SliderFloat("Sepia Amount", &postEffectData->sepiaColorAmount, 0.0f, 1.0f);
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
    if (postEffectData->modeFlags[0] & HEAT_HAZE)
    {
        ImGui::SliderFloat("Distortion Strength", &postEffectData->heatDistortionStrength, 0.0f, 0.05f);
        ImGui::SliderFloat("Noise Scale", &postEffectData->heatNoiseScale, 1.0f, 100.0f);
        ImGui::SliderFloat("Speed", &postEffectData->heatSpeed, 0.0f, 10.0f);
    }
    if (postEffectData->modeFlags[0] & WATER_REFRACTION)
    {
        ImGui::SliderFloat("Turbulent Strength", &postEffectData->turbulentStrength, 0.0f, 0.1f);
        ImGui::SliderFloat("Turbulent Frequency", &postEffectData->turbulentFrequency, 1.0f, 50.0f);
        ImGui::SliderFloat("Turbulent Speed", &postEffectData->turbulentSpeed, 0.0f, 10.0f);
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
    ImGui::Text("Models: %d / %d", engine_->renderer_->GetModelCount(), engine_->renderer_->kMaxModelCount);
    ImGui::Text("Sprites: %d / %d", engine_->renderer_->GetSpriteCount(), engine_->renderer_->kMaxSpriteCount);
    ImGui::Text("Lines: %d / %d", engine_->renderer_->GetLineCount(), engine_->renderer_->kMaxLineCount);
    ImGui::Text("Particles: %d / %d", engine_->renderer_->GetParticleCount(), engine_->renderer_->kMaxParticleCount);
    ImGui::Text("Trails: %d / %d", engine_->renderer_->GetTrailCount(), engine_->renderer_->kMaxTrailCount);

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
