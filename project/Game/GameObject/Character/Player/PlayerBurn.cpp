#include "pch.h"
#include "PlayerBurn.h"
#include "ImGuiManager.h"
#include "RendererManager.h"
#include "PostEffectManager.h"
#include "AudioPlayer.h"
#include "TimeManager.h"
#include "AnimationModel.h"

using namespace FS;

namespace
{

// 用意した音だけ鳴らす
void PlayIfLoaded(const char* name, float volume)
{
    AudioPlayer& audioPlayer = AudioPlayer::GetInstance();
    if (volume > 0.0f && audioPlayer.HasSound(name))
    {
        audioPlayer.Play(name, volume);
    }
}

}

PlayerBurn::PlayerBurn(Engine* engine)
    : engine_(engine)
{
}

void PlayerBurn::BindProperties(PropertyBinder& binder)
{
    config_.Bind(binder, "Burn_", BurnConfig{ 0.2f, 8.0f, 0.5f, 2.0f });

    binder.Bind("Burn_VignetteAmount", &vignetteAmount_, 0.5f, 0.01f, 0.0f, 1.0f);
    binder.Bind("Burn_FlickerAmount", &flickerAmount_, 0.15f, 0.01f, 0.0f, 1.0f);
    binder.Bind("Burn_FlickerSpeed", &flickerSpeed_, 9.0f, 0.1f, 0.0f, 50.0f);
    binder.BindColor("Burn_VignetteColor", &vignetteColor_, { 1.0f, 0.35f, 0.05f });
    binder.BindColor("Burn_VignetteHotColor", &vignetteHotColor_, { 0.85f, 0.05f, 0.02f });
    binder.Bind("Burn_VignetteStart", &vignetteStart_, 0.3f, 0.01f, 0.0f, 2.0f);
    binder.Bind("Burn_VignetteEnd", &vignetteEnd_, 0.95f, 0.01f, 0.0f, 2.0f);
    binder.Bind("Burn_FadeInTime", &fadeInTime_, 0.2f, 0.01f, 0.01f, 5.0f);
    binder.Bind("Burn_FadeOutTime", &fadeOutTime_, 0.6f, 0.01f, 0.01f, 5.0f);

    binder.Bind("Burn_FlashAmount", &flashAmount_, 0.35f, 0.01f, 0.0f, 1.0f);
    binder.Bind("Burn_FlashTime", &flashTime_, 0.25f, 0.01f, 0.01f, 3.0f);
    binder.Bind("Burn_VibrationStrength", &vibrationStrength_, 0.25f, 0.01f, 0.0f, 1.0f);
    binder.Bind("Burn_VibrationTime", &vibrationTime_, 0.1f, 0.01f, 0.0f, 2.0f);
    binder.Bind("Burn_CameraShake", &cameraShake_, 0.012f, 0.001f, 0.0f, 0.2f);
    binder.Bind("Burn_SoundVolume", &burnSoundVolume_, 0.5f, 0.01f, 0.0f, 2.0f);
    binder.Bind("Burn_VoiceVolume", &voiceVolume_, 0.5f, 0.01f, 0.0f, 2.0f);
}

void PlayerBurn::DrawProperties(PropertyBinder& binder)
{
    ImGui::Text("燃えている: %s / 強さ: %.2f", status_.IsBurning() ? "はい" : "いいえ", status_.GetStrength());

    ImGui::SeparatorText("ダメージ");
    config_.Draw(binder, "Burn_");

    ImGui::SeparatorText("画面の縁");
    binder.Draw("Burn_VignetteAmount", "強さ");
    binder.Draw("Burn_FlickerAmount", "ちらつきの量");
    binder.Draw("Burn_FlickerSpeed", "ちらつきの速さ");
    binder.Draw("Burn_VignetteColor", "色");
    binder.Draw("Burn_VignetteHotColor", "ちらつきで寄る色");
    binder.Draw("Burn_VignetteStart", "始まる距離 (画面中心から)");
    binder.Draw("Burn_VignetteEnd", "一番濃くなる距離");
    binder.Draw("Burn_FadeInTime", "燃え始めに出しきるまで (秒)");
    binder.Draw("Burn_FadeOutTime", "消えて戻るまで (秒)");

    ImGui::SeparatorText("ダメージが入った瞬間");
    binder.Draw("Burn_FlashAmount", "縁を強くする量");
    binder.Draw("Burn_FlashTime", "強くした縁が戻るまで (秒)");
    binder.Draw("Burn_VibrationStrength", "振動の強さ");
    binder.Draw("Burn_VibrationTime", "振動の長さ (秒)");
    binder.Draw("Burn_CameraShake", "カメラの揺れ");
    binder.Draw("Burn_SoundVolume", "燃えてダメージを受けたときの音の大きさ");
    binder.Draw("Burn_VoiceVolume", "うめき声 (playerBurnVoice) の大きさ");
}

int PlayerBurn::Update(const Vector3& footPosition, const AnimationModel* bodyModel, bool canTakeDamage)
{
    const float deltaTime = TimeManager::GetInstance()->GetDeltaTime();
    elapsedTime_ += deltaTime;

    int damage = 0;
    if (canTakeDamage)
    {
        damage = status_.Update(engine_->GetRendererManager(), footPosition, config_, deltaTime);
    }
    else
    {
        status_.Extinguish();
    }

    UpdateEmbers(bodyModel);
    ApplyVignette(deltaTime);
    return damage;
}

void PlayerBurn::OnDamaged()
{
    flashTimer_ = flashTime_;
    PlayIfLoaded("playerBurning", burnSoundVolume_);
    PlayIfLoaded("playerBurnVoice", voiceVolume_);
}

void PlayerBurn::Reset()
{
    status_.Extinguish();
    effectRatio_ = 0.0f;
    flashTimer_ = 0.0f;
    if (embersEmitter_ && isEmberPlaying_)
    {
        embersEmitter_->Stop();
    }
    isEmberPlaying_ = false;
}

void PlayerBurn::UpdateEmbers(const AnimationModel* bodyModel)
{
    const bool shouldPlay = status_.IsBurning();
    if (shouldPlay && !embersEmitter_)
    {
        embersEmitter_ = engine_->GetParticleSystem()->SpawnEmitter("playerBurnEmbers");
        if (embersEmitter_ && bodyModel)
        {
            embersEmitter_->SetTargetAnimationModel(bodyModel);
        }
    }
    if (!embersEmitter_ || shouldPlay == isEmberPlaying_)
    {
        return;
    }

    if (shouldPlay)
    {
        embersEmitter_->Play();
    }
    else
    {
        embersEmitter_->Stop();
    }
    isEmberPlaying_ = shouldPlay;
}

void PlayerBurn::ApplyVignette(float deltaTime)
{
    // 燃え始めははっきり、消えるときはゆっくり戻す
    if (status_.IsBurning())
    {
        effectRatio_ = std::min(effectRatio_ + deltaTime / fadeInTime_, 1.0f);
    }
    else
    {
        effectRatio_ = std::max(effectRatio_ - deltaTime / fadeOutTime_, 0.0f);
    }
    flashTimer_ = std::max(flashTimer_ - deltaTime, 0.0f);

    // 演出が無いときは、隠れているときの画面の縁をそのまま使う
    if (effectRatio_ <= 0.0f && flashTimer_ <= 0.0f)
    {
        return;
    }

    // 速さの違う揺れを重ねて、炎らしく不規則にちらつかせる
    const float flicker = 0.5f + 0.3f * std::sin(elapsedTime_ * flickerSpeed_) + 0.2f * std::sin(elapsedTime_ * flickerSpeed_ * 2.3f + 1.7f);
    const float easedRatio = effectRatio_ * effectRatio_ * (3.0f - 2.0f * effectRatio_);
    const float flash = (flashTime_ > 0.0f) ? flashAmount_ * (flashTimer_ / flashTime_) : 0.0f;
    const float amount = std::clamp(easedRatio * (vignetteAmount_ + flickerAmount_ * (flicker - 0.5f)) + flash, 0.0f, 1.0f);

    const float hotRatio = std::clamp(flicker + flash * 2.0f, 0.0f, 1.0f);
    const Vector3 color = vignetteColor_ + (vignetteHotColor_ - vignetteColor_) * hotRatio;

    if (PostEffectManager* postEffectManager = engine_->GetPostEffectManager())
    {
        postEffectManager->SetOverlayVignette(amount, color, vignetteStart_, vignetteEnd_);
    }
}
