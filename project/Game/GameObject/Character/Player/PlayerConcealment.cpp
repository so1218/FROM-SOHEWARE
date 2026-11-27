#include "pch.h"
#include "PlayerConcealment.h"
#include "GrassField.h"
#include "PostEffectManager.h"
#include "RendererManager.h"
#include "TimeManager.h"
#include "MathUtils.h"
#include "AudioPlayer.h"

using namespace FS;

PlayerConcealment::PlayerConcealment(Engine* engine) : engine_(engine)
{
}

void PlayerConcealment::BindProperties(PropertyBinder& binder)
{
    binder.Bind("HiddenBurntThreshold", &burntHideThreshold_, 0.3f, 0.01f, 0.0f, 1.0f);
    binder.Bind("HiddenVignetteAmount", &vignetteAmount_, 0.6f, 0.01f, 0.0f, 1.0f);
    binder.BindColor("HiddenVignetteColor", &vignetteColor_, { 0.01f, 0.02f, 0.03f });
    binder.Bind("HiddenVignetteStart", &vignetteStart_, 0.25f, 0.01f, 0.0f, 1.5f);
    binder.Bind("HiddenVignetteEnd", &vignetteEnd_, 0.85f, 0.01f, 0.0f, 1.5f);
    binder.Bind("HiddenFadeInTime", &fadeInTime_, 0.25f, 0.01f, 0.01f, 3.0f);
    binder.Bind("HiddenFadeOutTime", &fadeOutTime_, 0.5f, 0.01f, 0.01f, 3.0f);
    binder.Bind("HiddenWorldVolume", &hiddenWorldVolume_, 0.7f, 0.01f, 0.0f, 1.0f);
    binder.Bind("HiddenWorldMuffle", &hiddenWorldMuffle_, 0.6f, 0.01f, 0.0f, 1.0f);
}

void PlayerConcealment::DrawProperties(PropertyBinder& binder)
{
    ImGui::Text("草の中: %s", isInGrass_ ? "はい" : "いいえ");
    ImGui::Text("焼けた草の上: %s", isOnBurntGrass_ ? "はい" : "いいえ");
    ImGui::Text("置いた霧の中: %s", isInFogVolume_ ? "はい" : "いいえ");
    ImGui::Text("隠れている: %s", isHidden_ ? "はい" : "いいえ");

    binder.Draw("HiddenBurntThreshold", "隠れられなくなる焼け具合 (0 ~ 1)");
    binder.Draw("HiddenVignetteAmount", "ビネットの強さ");
    binder.Draw("HiddenVignetteColor", "ビネットの色");
    binder.Draw("HiddenVignetteStart", "ビネットが始まる距離 (画面中心から)");
    binder.Draw("HiddenVignetteEnd", "ビネットが一番暗くなる距離");
    binder.Draw("HiddenFadeInTime", "隠れたときに暗くなるまで (秒)");
    binder.Draw("HiddenFadeOutTime", "出たときに戻るまで (秒)");
    binder.Draw("HiddenWorldVolume", "隠れている間の周りの音の大きさ (1 で元のまま)");
    binder.Draw("HiddenWorldMuffle", "隠れている間の周りの音のこもり具合 (0 で元のまま)");
}

void PlayerConcealment::Update(const Vector3& footPosition, const Vector3& bodyCenter, bool isCrawling, bool isControllable)
{
    const RendererManager* rendererManager = engine_ ? engine_->GetRendererManager() : nullptr;
    const float burnt = rendererManager ? rendererManager->SampleGrassBurnt(footPosition.x, footPosition.z) : 0.0f;
    isOnBurntGrass_ = burnt > burntHideThreshold_;
    isInGrass_ = grassField_ && grassField_->IsGrassAt(footPosition.x, footPosition.z) && !isOnBurntGrass_;
    isInFogVolume_ = IsInsideFogVolume(bodyCenter);
    isHidden_ = isControllable && ((isCrawling && isInGrass_) || isInFogVolume_);

    // 入った瞬間ははっきり、出たときはゆっくり戻す
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    if (isHidden_)
    {
        effectRatio_ = std::min(effectRatio_ + deltaTime / fadeInTime_, 1.0f);
    }
    else
    {
        effectRatio_ = std::max(effectRatio_ - deltaTime / fadeOutTime_, 0.0f);
    }

    PostEffectManager* postEffectManager = engine_ ? engine_->GetPostEffectManager() : nullptr;
    if (postEffectManager)
    {
        const float easedRatio = effectRatio_ * effectRatio_ * (3.0f - 2.0f * effectRatio_);
        postEffectManager->SetOverlayVignette(vignetteAmount_ * easedRatio, vignetteColor_, vignetteStart_, vignetteEnd_);
    }

    // 周りの音を小さくこもらせる
    const float easedRatio = effectRatio_ * effectRatio_ * (3.0f - 2.0f * effectRatio_);
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    audioPlayer.SetWorldVolume(1.0f + (hiddenWorldVolume_ - 1.0f) * easedRatio);
    audioPlayer.SetWorldMuffle(hiddenWorldMuffle_ * easedRatio);
}

void PlayerConcealment::Reset()
{
    isInGrass_ = false;
    isOnBurntGrass_ = false;
    isInFogVolume_ = false;
    isHidden_ = false;
    effectRatio_ = 0.0f;

    PostEffectManager* postEffectManager = engine_ ? engine_->GetPostEffectManager() : nullptr;
    if (postEffectManager)
    {
        postEffectManager->SetOverlayVignette(0.0f, vignetteColor_);
    }
    AudioPlayer::GetInstance().SetWorldVolume(1.0f);
    AudioPlayer::GetInstance().SetWorldMuffle(0.0f);
}

bool PlayerConcealment::IsInsideFogVolume(const Vector3& position) const
{
    PostEffectManager* postEffectManager = engine_ ? engine_->GetPostEffectManager() : nullptr;
    if (!postEffectManager)
    {
        return false;
    }

    const FinalCompositeSettings* compositeSettings = postEffectManager->GetCompositeSettings();
    if (!compositeSettings || compositeSettings->enableVolumetricFog == 0)
    {
        return false;
    }

    for (const VolumetricFogPass::FogVolumeData& volume : postEffectManager->GetVolumetricFogPass()->GetFogVolumesData())
    {
        if (!volume.isVisible)
        {
            continue;
        }

        const Vector3 scale = (volume.type == 0)
            ? Vector3{ volume.scale.x, volume.scale.x, volume.scale.x }
            : volume.scale;

        const Matrix4x4 localToWorld =
            Matrix4x4::MakeScale(scale) *
            Matrix4x4::MakeRotateXYZ({
                Math::ToRadians(volume.rotation.x),
                Math::ToRadians(volume.rotation.y),
                Math::ToRadians(volume.rotation.z) }) *
            Matrix4x4::MakeTranslate(volume.position);
        const Vector3 localPosition = Matrix4x4::Inverse(localToWorld).TransformPoint(position);

        const bool isInside = (volume.type == 0)
            ? localPosition.Length() < 1.0f
            : (std::abs(localPosition.x) < 1.0f && std::abs(localPosition.y) < 1.0f && std::abs(localPosition.z) < 1.0f);
        if (isInside)
        {
            return true;
        }
    }

    return false;
}
