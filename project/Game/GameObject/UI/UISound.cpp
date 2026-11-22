#include "pch.h"
#include "UISound.h"
#include "AudioPlayer.h"

using namespace FS;

namespace
{
float gVolume = 0.5f;

void Play(const char* name)
{
    AudioPlayer::GetInstance().Play(name, gVolume);
}
}

namespace UISound
{

void BindProperties(PropertyBinder& binder)
{
    binder.Bind("UISoundVolume", &gVolume, 0.5f, 0.01f, 0.0f, 2.0f);
}

void DrawProperties(PropertyBinder& binder)
{
    binder.Draw("UISoundVolume", "操作音の大きさ (タイトル・ポーズ・ゲームオーバー共通)");
}

void PlayConfirm() { Play("uiConfirm"); }
void PlayCancel() { Play("uiCancel"); }
void PlayCursor() { Play("uiCursor"); }
void PlayPauseOpen() { Play("uiPauseOpen"); }
void PlayPauseClose() { Play("uiPauseClose"); }

}
