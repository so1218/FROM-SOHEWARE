#include "pch.h"
#include "DebugGuiManager.h"
#include "Engine.h"
#include "TimeManager.h"
#include "ImGuiManager.h"
#include "LightManager.h"
#include "MaterialManager.h"
#include "TextureLoader.h"
#include "PostEffectManager.h"
#include "DebugCamera.h"
#include "SRVManager.h"
#include "DebugDraw.h"

namespace FE
{

void DebugGuiManager::Initialize(Engine* engine, LightManager* lightManager, MaterialManager* materialManager,
    TextureLoader* textureLoader, PostEffectManager* postEffectManager, DebugCamera* debugCamera)
{
    engine_ = engine;
    lightManager_ = lightManager;
    materialManager_ = materialManager;
    textureLoader_ = textureLoader;
    postEffectManager_ = postEffectManager;
    debugCamera_ = debugCamera;
}

void DebugGuiManager::Update(Camera* targetCamera)
{
#ifdef ENABLE_IMGUI
    // メインのデバッグウィンドウ
    ImGui::Begin("全体のデバッグ情報・設定");

    if (ImGui::CollapsingHeader("描画系設定"))
    {
        DrawRenderSettings();
    }
    if (ImGui::CollapsingHeader("カメラ設定"))
    {
        DrawCameraSettings(targetCamera);
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

    if (lightManager_)
    {
        lightManager_->DrawSelectedLightGizmo();
    }

#endif
}

#ifdef ENABLE_IMGUI
void DebugGuiManager::DrawRenderSettings()
{
    ImGui::Checkbox("ワイヤーフレーム描画", &engine_->GetRendererManager()->isWireFrame_);
}

void DebugGuiManager::DrawCameraSettings(Camera* targetCamera)
{
    bool enabled = engine_->GetDebugCamera()->IsEnabled();
    if (ImGui::Checkbox("デバッグカメラを有効化", &enabled))
    {
        engine_->GetDebugCamera()->SetEnabled(enabled);
    }

    if (targetCamera && ImGui::TreeNode("ターゲットカメラ (Scene)"))
    {
        Vector3 translation = targetCamera->GetTranslation();
        Vector3 rotationEuler = targetCamera->GetWorldRotationEuler();
        float fov = targetCamera->GetFov();
        float nearClip = targetCamera->GetNearClip();
        float farClip = targetCamera->GetFarClip();

        if (ImGui::DragFloat3("座標 (World)", &translation.x, 0.1f))
        {
            targetCamera->SetTranslation(translation);
            targetCamera->UpdateViewMatrix();
        }

        if (ImGui::DragFloat3("回転 (World)", &rotationEuler.x, 0.1f))
        {
            targetCamera->SetWorldRotationEuler(rotationEuler);
            targetCamera->UpdateViewMatrix();
        }

        if (ImGui::DragFloat("視野角 (FOV)", &fov, 0.1f, 1.0f, 179.0f))
        {
            targetCamera->SetFov(fov);
        }
        if (ImGui::DragFloat("ニアクリップ", &nearClip, 0.01f, 0.001f, 100.0f))
        {
            targetCamera->SetNearClip(nearClip);
        }
        if (ImGui::DragFloat("ファークリップ", &farClip, 1.0f, 1.0f, 10000.0f))
        {
            targetCamera->SetFarClip(farClip);
        }

        targetCamera->UpdateProjectionMatrix();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("デバッグカメラ設定"))
    {
        Vector3 target = debugCamera_->GetTarget();
        if (ImGui::DragFloat3("注視点", &target.x, 0.1f))
        {
            debugCamera_->SetTarget(target);
        }

        float distance = debugCamera_->GetDistance();
        if (ImGui::DragFloat("注視点からの距離", &distance, 0.1f, 1.0f, 500.0f))
        {
            debugCamera_->SetDistance(distance);
        }

        float pitch = debugCamera_->GetCurrentPitch();
        if (ImGui::DragFloat("ピッチ (縦回転)", &pitch, 0.1f, -89.0f, 89.0f))
        {
            debugCamera_->SetCurrentPitch(pitch);
        }

        float yaw = debugCamera_->GetCurrentYaw();
        if (ImGui::DragFloat("ヨー (横回転)", &yaw, 0.1f, -180.0f, 180.0f))
        {
            debugCamera_->SetCurrentYaw(yaw);
        }

        float dragSpeed = debugCamera_->GetDragSpeed();
        if (ImGui::DragFloat("ドラッグ速度", &dragSpeed, 0.001f, 0.001f, 1.0f))
        {
            debugCamera_->SetDragSpeed(dragSpeed);
        }

        float rotateSpeed = debugCamera_->GetRotateSpeed();
        if (ImGui::DragFloat("回転速度", &rotateSpeed, 0.0001f, 0.0001f, 0.05f))
        {
            debugCamera_->SetRotateSpeed(rotateSpeed);
        }

        float zoomSpeed = debugCamera_->GetZoomSpeed();
        if (ImGui::DragFloat("ズーム速度", &zoomSpeed, 0.001f, 0.01f, 1.0f))
        {
            debugCamera_->SetZoomSpeed(zoomSpeed);
        }

        ImGui::TreePop();
    }

}

void DebugGuiManager::DrawLightSettings()
{
    DirectionalLight* dirLights = lightManager_->GetDirectionalLightData();
    PointLight* pointLights = lightManager_->GetPointLightData();
    SpotLight* spotLights = lightManager_->GetSpotLightData();
    AreaLight* areaLights = lightManager_->GetAreaLightData();

    ImGui::Separator();

    bool hasSelection = (lightManager_->GetSelectedLightType() != SelectedLightType::None);

    // 選択がない場合はボタンを無効化
    if (!hasSelection) ImGui::BeginDisabled();

    if (ImGui::Button("Gizmoを非表示", ImVec2(-1, 0)))
    {
        lightManager_->SetSelectedLight(SelectedLightType::None, -1);
    }

    if (!hasSelection) ImGui::EndDisabled();

    ImGui::Separator();

    // ディレクショナルライト
    if (ImGui::TreeNode("ディレクショナルライト (平行光源)"))
    {
        for (int i = 0; i < lightManager_->GetDirectionalLightCount(); ++i)
        {
            std::string label = "ディレクショナルライト " + std::to_string(i);

            // 現在のこのライトが選択されているかチェック
            bool isSelected = (lightManager_->GetSelectedLightType() == SelectedLightType::Directional &&
                lightManager_->GetSelectedLightIndex() == i);

            // ツリーノードのフラグ設定（選択されている場合はハイライト色にする）
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // TreeNodeEx でノードを描画
            bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

            // 項目がクリックされたら、選択状態を更新
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                lightManager_->SetSelectedLight(SelectedLightType::Directional, i);
            }

            // ツリーが開かれているならパラメータを表示
            if (isOpen)
            {
                bool enabled = (dirLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled))
                {
                    dirLights[i].enable = enabled ? 1 : 0;
                }
                ImGui::DragFloat3("向き", &dirLights[i].direction.x, 0.05f);
                ImGui::ColorEdit4("色", &dirLights[i].color.x);
                ImGui::DragFloat("強度", &dirLights[i].intensity, 0.01f, 0.0f, 100.0f);
                ImGui::DragFloat("ボリュメトリック散乱強度", &dirLights[i].volumetricScatteringIntensity, 0.05f, 0.0f, 50.0f);
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

            // 現在のこのライトが選択されているかチェック
            bool isSelected = (lightManager_->GetSelectedLightType() == SelectedLightType::Point &&
                lightManager_->GetSelectedLightIndex() == i);

            // ツリーノードのフラグ設定
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // TreeNodeEx でノードを描画
            bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

            // 項目がクリックされたら、選択状態を更新
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                lightManager_->SetSelectedLight(SelectedLightType::Point, i);
            }

            // ツリーが開かれているならパラメータを表示
            if (isOpen)
            {
                bool enabled = (pointLights[i].enable != 0);
                if (ImGui::Checkbox("有効", &enabled)) { pointLights[i].enable = enabled ? 1 : 0; }
                ImGui::DragFloat3("座標", &pointLights[i].position.x, 0.05f);
                ImGui::ColorEdit4("色", &pointLights[i].color.x);
                ImGui::DragFloat("強度", &pointLights[i].intensity, 0.01f);
                ImGui::DragFloat("影響半径", &pointLights[i].radius, 0.1f);
                ImGui::DragFloat("ボリュームフォグ輝度", &pointLights[i].volumetricScatteringIntensity, 0.05f, 0.0f, 50.0f);

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

            // 現在のこのライトが選択されているかチェック
            bool isSelected = (lightManager_->GetSelectedLightType() == SelectedLightType::Spot &&
                lightManager_->GetSelectedLightIndex() == i);

            // ツリーノードのフラグ設定
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // TreeNodeEx でノードを描画
            bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

            // 項目がクリックされたら、選択状態を更新
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                lightManager_->SetSelectedLight(SelectedLightType::Spot, i);
            }

            // ツリーが開かれているならパラメータを表示
            if (isOpen)
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
                ImGui::DragFloat("照射角(コサイン値)", &spotLights[i].cosAngle, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("ボリュームフォグ輝度", &spotLights[i].volumetricScatteringIntensity, 0.05f, 0.0f, 50.0f);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Separator();

    // エリアライト 
    if (ImGui::TreeNode("エリアライト (矩形光源)"))
    {
        for (int i = 0; i < lightManager_->GetAreaLightCount(); ++i)
        {
            std::string label = "エリアライト " + std::to_string(i);

            // 現在のこのライトが選択されているかチェック
            bool isSelected = (lightManager_->GetSelectedLightType() == SelectedLightType::Area &&
                lightManager_->GetSelectedLightIndex() == i);

            // ツリーノードのフラグ設定
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // TreeNodeEx でノードを描画
            bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

            // 項目がクリックされたら、選択状態を更新
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                lightManager_->SetSelectedLight(SelectedLightType::Area, i);
            }

            // ツリーが開かれているならパラメータを表示
            if (isOpen)
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
}

void DebugGuiManager::DrawPostEffectSettings()
{
    // 各データへのポインタ取得
    PostEffectData* postEffectData = postEffectManager_->GetPostEffectData();
    BrightExtractSettings* brightExtractData = postEffectManager_->GetBrightSettings();
    BloomSettings* bloomSettings = postEffectManager_->GetBloomSettings();
    FinalCompositeSettings* compositeSettingsData = postEffectManager_->GetCompositeSettings();
    VolumetricFogSettings* volFogSettings = postEffectManager_->GetVolumetricFogSettings();
    FogBilateralSettings* fogBilateralSettings = postEffectManager_->GetFogBilateralSettings();
    DoFSettings* dofSettings = postEffectManager_->GetDoFSettings();
    SSAOSettings* ssaoSettings = postEffectManager_->GetSSAOSettings();
    BilateralBlurSettings* bilateralSettings = postEffectManager_->GetBilateralBlurSettings();
    SSRSettings* ssrSettings = postEffectManager_->GetSSRSettings();
    std::vector<VolumetricFogPass::FogVolumeData>& volumes = postEffectManager_->GetVolumetricFogPass()->GetFogVolumesData();

    // カラー・色調系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "カラー・色調");

    // グレースケール
    if (ImGui::CheckboxFlags("グレースケール", &postEffectData->flag[0], GRAYSCALE)) {}
    if (postEffectData->flag[0] & GRAYSCALE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("適用量##Gray", &postEffectData->grayscaleColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // セピア
    if (ImGui::CheckboxFlags("セピア", &postEffectData->flag[0], SEPIA)) {}
    if (postEffectData->flag[0] & SEPIA)
    {
        ImGui::Indent();
        ImGui::SliderFloat("適用量##Sepia", &postEffectData->sepiaColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // カラーティント (色合い調整)
    if (ImGui::CheckboxFlags("カラーティント", &postEffectData->flag[0], COLOR_TINT)) {}
    if (postEffectData->flag[0] & COLOR_TINT)
    {
        ImGui::Indent();
        ImGui::ColorEdit3("着色カラー", &postEffectData->tintColor.x);
        ImGui::SliderFloat("乗算合成率", &postEffectData->tintMulColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("加算合成率", &postEffectData->tintAddColorAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("スクリーン合成率", &postEffectData->tintScreenColorAmount, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // ヴィネット
    if (ImGui::CheckboxFlags("ビネット", &postEffectData->flag[0], VIGNETTE)) {}
    if (postEffectData->flag[0] & VIGNETTE)
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
    if (ImGui::CheckboxFlags("色収差", &postEffectData->flag[0], CHROM_ABERRATION)) {}
    if (postEffectData->flag[0] & CHROM_ABERRATION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ズレ幅", &postEffectData->chromaOffset, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // RGBずらし
    if (ImGui::CheckboxFlags("RGBずらし", &postEffectData->flag[0], RGB_SPLIT)) {}
    if (postEffectData->flag[0] & RGB_SPLIT)
    {
        ImGui::Indent();
        ImGui::SliderFloat("オフセット量", &postEffectData->rgbSplitOffset, 0.0f, 0.05f);
        ImGui::Unindent();
    }

    ImGui::Separator();

    // 形状・歪み系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "形状・歪み");

    // ドット化
    if (ImGui::CheckboxFlags("ドット化", &postEffectData->flag[0], PIXELATION)) {}
    if (postEffectData->flag[0] & PIXELATION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ドットサイズ", &postEffectData->pixelationSize, 1.0f, 64.0f);
        ImGui::Unindent();
    }

    // 画面の波紋
    if (ImGui::CheckboxFlags("画面の波紋 (Wave)", &postEffectData->flag[0], SCREEN_WAVE)) {}
    if (postEffectData->flag[0] & SCREEN_WAVE)
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
    if (ImGui::CheckboxFlags("魚眼レンズ", &postEffectData->flag[0], FISHEYE)) {}
    if (postEffectData->flag[0] & FISHEYE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("歪み強度", &postEffectData->fisheyeDistortion, 0.0f, 2.0f);
        ImGui::Unindent();
    }

    // ヒートヘイズ
    if (ImGui::CheckboxFlags("ヒートヘイズ (陽炎)", &postEffectData->flag[0], HEAT_HAZE)) {}
    if (postEffectData->flag[0] & HEAT_HAZE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("歪み強度", &postEffectData->heatDistortionStrength, 0.0f, 0.05f);
        ImGui::SliderFloat("ノイズスケール", &postEffectData->heatNoiseScale, 1.0f, 100.0f);
        ImGui::SliderFloat("ゆらぎ速度", &postEffectData->heatSpeed, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // 水面屈折
    if (ImGui::CheckboxFlags("水面屈折", &postEffectData->flag[0], WATER_REFRACTION)) {}
    if (postEffectData->flag[0] & WATER_REFRACTION)
    {
        ImGui::Indent();
        ImGui::SliderFloat("乱流強度", &postEffectData->turbulentStrength, 0.0f, 0.1f);
        ImGui::SliderFloat("乱流周波数", &postEffectData->turbulentFrequency, 1.0f, 50.0f);
        ImGui::SliderFloat("速度", &postEffectData->turbulentSpeed, 0.0f, 10.0f);
        ImGui::Unindent();
    }

    // ラディアルブラー
    if (ImGui::CheckboxFlags("ラディアルブラー", &postEffectData->flag[0], RADIAL_BLUR)) {}
    if (postEffectData->flag[0] & RADIAL_BLUR)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ブラー強度", &postEffectData->radialBlurStrength, 0.0f, 0.2f);
        ImGui::SliderFloat2("中心座標 (UV)", &postEffectData->radialBlurCenter.x, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    ImGui::Separator();

    // 特殊効果・ノイズ系
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "特殊効果・ノイズ");

    // 走査線
    if (ImGui::CheckboxFlags("走査線 (Scanline)", &postEffectData->flag[0], SCANLINE)) {}
    if (postEffectData->flag[0] & SCANLINE)
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
    if (ImGui::CheckboxFlags("スクリーンノイズ (砂嵐)", &postEffectData->flag[0], SCREEN_NOISE)) {}
    if (postEffectData->flag[0] & SCREEN_NOISE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ノイズ量", &postEffectData->noiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("変化速度", &postEffectData->noiseSpeed, 0.0f, 10.0f);
        ImGui::SliderFloat("粒度スケール", &postEffectData->noiseScale, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // ブロックノイズ
    if (ImGui::CheckboxFlags("ブロックノイズ", &postEffectData->flag[0], BLOCK_NOISE)) {}
    if (postEffectData->flag[0] & BLOCK_NOISE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ノイズ量", &postEffectData->blockNoiseAmount, 0.0f, 1.0f);
        ImGui::SliderFloat("ブロックサイズ", &postEffectData->blockNoiseSize, 4.0f, 128.0f);
        ImGui::SliderFloat("変化速度", &postEffectData->blockNoiseSpeed, 0.0f, 100.0f);
        ImGui::Unindent();
    }

    // フィルムグレイン
    if (ImGui::CheckboxFlags("フィルムグレイン (粒子)", &postEffectData->flag[0], FILM_GRAIN)) {}
    if (postEffectData->flag[0] & FILM_GRAIN)
    {
        ImGui::Indent();
        ImGui::SliderFloat("粒子強度", &postEffectData->filmGrainIntensity, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    // グリッチ
    if (ImGui::CheckboxFlags("グリッチエフェクト", &postEffectData->flag[0], GLITCH)) {}
    if (postEffectData->flag[0] & GLITCH)
    {
        ImGui::Indent();
        ImGui::SliderFloat("ブロックの高さ", &postEffectData->glitchBlockHeight, 0.01f, 0.2f);
        ImGui::SliderFloat("グリッチ量", &postEffectData->glitchAmount, 0.0f, 0.3f);
        ImGui::SliderFloat("ノイズ強度", &postEffectData->glitchNoiseIntensity, 0.0f, 0.5f);
        ImGui::Unindent();
    }

    // ディゾルブ
    if (ImGui::CheckboxFlags("ディゾルブ", &postEffectData->flag[0], DISSOLVE)) {}
    if (postEffectData->flag[0] & DISSOLVE)
    {
        ImGui::Indent();
        ImGui::SliderFloat("進行度", &postEffectData->dissolveThreshold, 0.0f, 1.0f);
        ImGui::SliderFloat("境界の幅", &postEffectData->dissolveEdgeWidth, 0.0f, 0.2f);
        ImGui::DragFloat("発光強度", &postEffectData->dissolveEdgeIntensity, 0.1f, 0.0f, 50.0f);
        ImGui::ColorEdit3("境界色", &postEffectData->dissolveEdgeColor.x);
        ImGui::Unindent();
    }

    if (ImGui::CheckboxFlags("カラーグレーディング (LUT)", &postEffectData->flag[0], COLOR_GRADING_LUT)) {}
    if (postEffectData->flag[0] & COLOR_GRADING_LUT)
    {
        ImGui::Indent();

        std::vector<std::string> lutNames = TextureManager::GetInstance().GetTextureNamesByType(TextureType::LUT);

        if (lutNames.empty())
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "LUTテクスチャがない");
        }
        else
        {
            // PostEffectManager から現在の名前を取得
            std::string currentLut = postEffectManager_->GetCurrentLutName();

            if (ImGui::BeginCombo("LUTテクスチャ", currentLut.c_str()))
            {
                for (const auto& name : lutNames)
                {
                    bool isSelected = (currentLut == name);

                    if (ImGui::Selectable(name.c_str(), isSelected))
                    {
                        // 選択されたら、Manager側に保存する
                        postEffectManager_->SetCurrentLutName(name);
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Unindent();
    }

    // 環境・光・深度設定
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "環境・光・深度設定");

    // SSAO
    if (ImGui::TreeNode("環境遮蔽 (SSAO)"))
    {
        bool ssaoFlag = (compositeSettingsData->enableSSAO != 0);
        if (ImGui::Checkbox("SSAO有効", &ssaoFlag))
        {
            compositeSettingsData->enableSSAO = ssaoFlag ? 1 : 0;
        }

        if (ssaoFlag && ssaoSettings)
        {
            ImGui::Indent();

            ImGui::SliderFloat("サンプリング半径", &ssaoSettings->radius, 0.1f, 50.0f, "%.1f");
            ImGui::SliderFloat("影の濃さ", &ssaoSettings->intensity, 0.0f, 10.0f, "%.2f");
            ImGui::DragFloat("バイアス", &ssaoSettings->bias, 0.001f, 0.0f, 1.0f, "%.4f");

            ImGui::Separator();

            ImGui::SliderInt("サンプル数", &ssaoSettings->sampleCount, 4, 64);
            ImGui::DragFloat("フェード開始距離", &ssaoSettings->fadeStart, 1.0f, 0.0f, 1000.0f, "%.1f");
            ImGui::DragFloat("フェード終了距離", &ssaoSettings->fadeEnd, 1.0f, 0.0f, 1000.0f, "%.1f");

            if (ssaoSettings->fadeStart > ssaoSettings->fadeEnd)
            {
                ssaoSettings->fadeEnd = ssaoSettings->fadeStart + 0.1f;
            }

            if (bilateralSettings)
            {
                ImGui::Separator();
                ImGui::TextDisabled("ノイズ除去 (Bilateral Blur)");
                ImGui::SliderFloat("深度の許容度", &bilateralSettings->depthTolerance, 0.0f, 100.0f, "%.3f");
                ImGui::SliderFloat("法線の許容度", &bilateralSettings->normalTolerance, 0.0f, 256.0f, "%.1f");
            }
          
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    // ブルーム設定
    if (ImGui::TreeNode("ブルーム"))
    {
        if (ImGui::TreeNode("輝度抽出"))
        {
            ImGui::SliderFloat("しきい値", &brightExtractData->threshold, 0.0f, 10.0f);
            ImGui::SliderFloat("抽出強度", &brightExtractData->intensity, 0.0f, 5.0f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("光の広がり"))
        {
            ImGui::SliderFloat("拡散半径", &bloomSettings->radius, 0.0f, 5.0f, "%.2f");
            ImGui::TreePop();
        }

        ImGui::Text("ブルーム合成強度");
        ImGui::SliderFloat("Intensity", &compositeSettingsData->bloomIntensity, 0.0f, 5.0f);

        ImGui::TreePop();
    }

    // 被写界深度 (DoF)
    if (ImGui::TreeNode("被写界深度 (DoF)"))
    {
        bool dofFlag = (compositeSettingsData->enableDoF != 0);
        if (ImGui::Checkbox("DoF有効", &dofFlag))
            compositeSettingsData->enableDoF = dofFlag ? 1 : 0;

        if (dofFlag)
        {
            ImGui::Indent();
            ImGui::SliderFloat("ピント距離", &dofSettings->focusDistance, 0.1f, 500.0f, "%.1f m");
            ImGui::SliderFloat("ピント範囲 (ボケない幅)", &dofSettings->focusRange, 0.1f, 100.0f, "%.1f m");
            ImGui::SliderFloat("ボケの強さ (半径)", &dofSettings->bokehRadius, 1.0f, 50.0f, "%.1f px");

            ImGui::Separator();
            ImGui::SliderFloat("ボケ移行距離", &dofSettings->transitionRange, 0.1f, 100.0f, "%.1f m");
            ImGui::SliderFloat("玉ボケ閾値 (明るさ)", &dofSettings->bokehHighlightThreshold, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("玉ボケ強度", &dofSettings->bokehHighlightIntensity, 0.0f, 200.0f, "%.1f");

            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("反射 (SSR)"))
    {
        bool ssrFlag = (compositeSettingsData->enableSSR != 0);
        if (ImGui::Checkbox("SSR有効", &ssrFlag))
        {
            compositeSettingsData->enableSSR = ssrFlag ? 1 : 0;
        }

        if (ssrFlag)
        {
            ImGui::Indent();

            ImGui::TextDisabled("合成設定");
            ImGui::DragFloat("反射強度", &compositeSettingsData->ssrIntensity, 0.01f);

            ImGui::Separator();

            if (ssrSettings)
            {
                ImGui::TextDisabled("生成パラメータ");

                ImGui::DragFloat("最大探索距離", &ssrSettings->maxDistance, 0.5f, 0.0f, 1000.0f, "%.1f m");
                ImGui::DragFloat("ステップサイズ", &ssrSettings->stepSize, 0.01f, 0.01f, 10.0f, "%.3f");
                ImGui::SliderInt("最大ステップ数", &ssrSettings->maxSteps, 1, 256);
                ImGui::DragFloat("厚み判定", &ssrSettings->thickness, 0.01f, 0.0f, 5.0f, "%.3f");
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "SSRSettingsのポインタがnull");
            }

            ImGui::Unindent();
        }
        ImGui::TreePop();
    }

    // フォグ設定
    if (ImGui::TreeNode("ボリュメトリックフォグ"))
    {
        bool volFogFlag = (compositeSettingsData->enableVolumetricFog != 0);
        if (ImGui::Checkbox("有効にする", &volFogFlag))
        {
            compositeSettingsData->enableVolumetricFog = volFogFlag ? 1 : 0;
        }

        if (volFogFlag && volFogSettings)
        {
            ImGui::Indent();

            if (ImGui::CollapsingHeader("PBR 光学特性", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3("散乱色", &volFogSettings->albedo.x);
                ImGui::DragFloat("散乱の強さ (Scattering Intensity)", &volFogSettings->scatteringIntensity, 0.5f, 0.0f, 200.0f, "%.1f");
                ImGui::DragFloat("光の減衰スケール (Extinction)", &volFogSettings->extinctionScale, 0.01f, 0.0f, 10.0f, "%.2f");
                ImGui::SliderFloat("前方散乱・光の筋 (Anisotropy)", &volFogSettings->anisotropy, -0.99f, 0.99f, "%.2f");

                ImGui::Separator();
                ImGui::ColorEdit3("環境光", &volFogSettings->ambientLight.x);
            }

            if (ImGui::CollapsingHeader("密度と形状", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("空間全体のベース密度", &volFogSettings->extinction, 0.001f, 0.0f, 1.0f, "%.3f");
                ImGui::DragFloat("高さフォグの最大密度", &volFogSettings->heightDensity, 0.01f, 0.0f, 5.0f, "%.2f");
                ImGui::DragFloat("基準高さ", &volFogSettings->baseHeight, 0.1f, -100.0f, 100.0f, "%.1f");
                ImGui::DragFloat("高さ減衰率", &volFogSettings->heightFalloff, 0.001f, 0.0f, 1.0f, "%.3f");
            }

            if (ImGui::CollapsingHeader("ノイズ設定"))
            {
                ImGui::DragFloat("ノイズスケール", &volFogSettings->noiseScale, 0.001f, 0.0f, 1.0f, "%.3f");
                ImGui::SliderFloat("ノイズ歪み", &volFogSettings->noiseDistortion, 0.0f, 1.0f, "%.2f");
                ImGui::DragFloat3("風向き", &volFogSettings->windDirection.x, 0.1f, -1.0f, 1.0f);
                ImGui::DragFloat("風速", &volFogSettings->windSpeed, 0.01f, -5.0f, 5.0f, "%.2f");
                ImGui::Separator();
                ImGui::Text("形状コントロール");
                ImGui::SliderFloat("霧の量 (Coverage)", &volFogSettings->coverage, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("雲の塊感 (Worley Weight)", &volFogSettings->worleyWeight, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("削り取り強度 (Erosion)", &volFogSettings->erosion, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("流体との融合削り強度 (Erosion Strength)", &volFogSettings->erosionStrength, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("全体ノイズ適用度 (Noise Intensity)", &volFogSettings->noiseIntensity, 0.0f, 1.0f, "%.2f");
                ImGui::DragFloat("境界のボケ具合 (Feather)", &volFogSettings->noiseFeather, 0.01f, 0.001f, 2.0f, "%.3f");
            }

            if (ImGui::CollapsingHeader("システム・TAA"))
            {
                ImGui::DragFloat("最大描画距離", &volFogSettings->maxDistance, 1.0f, 10.0f, 5000.0f, "%.0f m");
                ImGui::SliderFloat("TAA蓄積ウェイト", &volFogSettings->temporalWeight, 0.01f, 0.5f, "%.3f");
                ImGui::Text("ボクセル解像度 (Z): %.0f", volFogSettings->depthSliceCount);
            }

            if (fogBilateralSettings && ImGui::CollapsingHeader("ノイズ除去"))
            {
                ImGui::DragInt("半径", &fogBilateralSettings->blurRadius, 1, 1, 5);
                ImGui::DragFloat("Spatial Sigma", &fogBilateralSettings->spatialSigma, 0.1f, 0.1f, 10.0f);
                ImGui::DragFloat("Depth Sigma", &fogBilateralSettings->depthSigma, 0.0001f, 0.00001f, 0.1f, "%.5f");
            }

            if (ImGui::CollapsingHeader("配置式フォグ"))
            {
                if (ImGui::Button("ボリュームを追加"))
                {
                    if (volumes.size() < MAX_FOG_VOLUMES) volumes.push_back(VolumetricFogPass::FogVolumeData());
                }

                for (int i = 0; i < volumes.size(); ++i)
                {
                    auto& vol = volumes[i];
                    ImGui::PushID(static_cast<int>(i));

                    if (ImGui::TreeNode(("Volume " + std::to_string(i)).c_str()))
                    {
                        ImGui::Checkbox("デバッグ描画", &vol.isVisible);
                        ImGui::Combo("タイプ", &vol.type, "Sphere\0Box\0");

                        ImGui::DragFloat3("位置", &vol.position.x, 0.1f);
                        ImGui::DragFloat3("回転", &vol.rotation.x, 1.0f);
                        if (vol.type == 0) {
                            ImGui::DragFloat("半径", &vol.scale.x, 0.1f, 0.1f, 1000.0f);
                        }
                        else {
                            ImGui::DragFloat3("サイズ", &vol.scale.x, 0.1f, 0.1f, 1000.0f);
                        }

                        ImGui::ColorEdit3("色", &vol.color.x);
                        ImGui::DragFloat("密度", &vol.density, 0.01f, 0.0f, 10.0f);

                        ImGui::SliderFloat("境界ボカシ (Blend)", &vol.blendDistance, 0.0f, 1.0f);

                        ImGui::Separator();
                        ImGui::Text("Volume 専用光学特性");
                        ImGui::SliderFloat("光の筋 (Anisotropy)", &vol.anisotropy, -0.99f, 0.99f);
                        ImGui::DragFloat3("風向き", &vol.windDirection.x, 0.1f, -1.0f, 1.0f);
                        ImGui::DragFloat("風速", &vol.windSpeed, 0.01f, -5.0f, 5.0f);
                        ImGui::DragFloat3("ノイズスケール", &vol.noiseScale.x, 0.01f);
                        ImGui::SliderFloat("ノイズ強度", &vol.noiseIntensity, 0.0f, 1.0f);

                        ImGui::Separator();
                        ImGui::Text("Volume 形状コントロール");
                        ImGui::SliderFloat("霧の量 (Coverage)", &vol.coverage, 0.0f, 1.0f);
                        ImGui::SliderFloat("雲の塊感 (Worley Weight)", &vol.worleyWeight, 0.0f, 1.0f);
                        ImGui::SliderFloat("削り取り強度 (Erosion)", &vol.erosion, 0.0f, 1.0f);
                        ImGui::DragFloat("境界のボケ具合 (Feather)", &vol.noiseFeather, 0.01f, 0.001f, 2.0f);

                        ImGui::Separator();
                        ImGui::Text("Volume ディテール制御");
                        ImGui::SliderFloat("流体歪み強さ (Distortion)", &vol.distortionAmount, 0.0f, 1.0f);
                        ImGui::SliderFloat("密度の底上げ (Density Offset)", &vol.densityOffset, -1.0f, 1.0f);
                        ImGui::DragFloat("コントラスト", &vol.noiseContrast, 0.05f, 0.0f, 10.0f);
                        ImGui::DragFloat("ローカル高さ減衰 (Height Falloff)", &vol.heightFalloff, 0.05f, 0.0f, 10.0f);

                        ImGui::TreePop();
                    }
                    ImGui::PopID();

                    if (vol.isVisible)
                    {
                        Vector4 drawColor = { vol.color.x, vol.color.y, vol.color.z, 1.0f };

                        if (vol.type == 0) 
                        {
                            DebugDraw::DrawSphere(vol.position, vol.scale.x, drawColor);
                        }
                        else if (vol.type == 1) 
                        {
                            Matrix4x4 rotMat = Matrix4x4::MakeRotateXYZ({
                                Math::ToRadians(vol.rotation.x),
                                Math::ToRadians(vol.rotation.y),
                                Math::ToRadians(vol.rotation.z) }
                            );
                            Vector3 fullSize = { vol.scale.x * 2.0f, vol.scale.y * 2.0f, vol.scale.z * 2.0f };
                            DebugDraw::DrawOBB(vol.position, fullSize, rotMat, drawColor);
                        }
                    }
                }
            }

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
    ImGui::Text("Models: %d / %d", engine_->GetRendererManager()->GetModelCount(), engine_->GetRendererManager()->GetMaxModelCount());
    ImGui::Text("Sprites: %d / %d", engine_->GetRendererManager()->GetSpriteCount(), engine_->GetRendererManager()->GetMaxSpriteCount());
    ImGui::Text("Lines: %d / %d", engine_->GetRendererManager()->GetLineCount(), engine_->GetRendererManager()->GetMaxLineCount());
    ImGui::Text("Particles: %d / %d", engine_->GetRendererManager()->GetParticleCount(), engine_->GetRendererManager()->GetMaxParticleCount());
    ImGui::Text("Trails: %d / %d", engine_->GetRendererManager()->GetTrailCount(), engine_->GetRendererManager()->GetMaxTrailCount());
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

    // ウィンドウの利用可能なサイズを取得
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // ゲームの解像度のアスペクト比を計算
    float targetAspect = static_cast<float>(Engine::GetClientWidth()) / static_cast<float>(Engine::GetClientHeight());

    // ウィンドウのアスペクト比を計算
    float windowAspect = windowSize.x / windowSize.y;

    // アスペクト比に合わせて描画サイズを計算
    ImVec2 finalSize = windowSize;
    if (windowAspect > targetAspect)
    {
        // ウィンドウの方が横長 → 高さに合わせる
        finalSize.x = windowSize.y * targetAspect;
    }
    else
    {
        // ウィンドウの方が縦長 → 幅に合わせる
        finalSize.y = windowSize.x / targetAspect;
    }

    // 画像を中央に寄せるためのオフセット計算
    ImVec2 cursorStart = ImGui::GetCursorPos();
    ImVec2 offset;
    offset.x = (windowSize.x - finalSize.x) * 0.5f;
    offset.y = (windowSize.y - finalSize.y) * 0.5f;

    // カーソル位置をずらして画像を描画
    ImGui::SetCursorPos(ImVec2(cursorStart.x + offset.x, cursorStart.y + offset.y));

    ImGui::Image(reinterpret_cast<ImTextureID>(reinterpret_cast<void*>(gpuHandle.ptr)), finalSize);

    // 座標計算
    ImVec2 vMin = ImGui::GetItemRectMin();
    ImVec2 vMax = ImGui::GetItemRectMax();
    bool isHovered = ImGui::IsItemHovered();

    // ImGuizmoのセットアップ 
    ImGuizmo::SetRect(vMin.x, vMin.y, vMax.x - vMin.x, vMax.y - vMin.y);
    ImGuizmo::SetDrawlist();

    // 計算結果をImGuiManagerへ
    ImGuiManager::SetSceneViewRect(
        Vector2(vMin.x, vMin.y),
        Vector2(vMax.x - vMin.x, vMax.y - vMin.y),
        isHovered
    );
}

void DebugGuiManager::EndSceneView()
{
    // ウィンドウを閉じるだけ
    ImGui::End();
}

#endif

}