#include "pch.h"
#include "FadeOutState.h"
#include "FadeInState.h"

void FadeOutState::Update(SceneManager* manager)
{
    // フェードの更新
    auto* fade = manager->GetFade();
    fade->Update();

    // 現在のシーン
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Update();
    }

    // フェードアウト終了判定
    if (fade->IsFinished())
    {
        // シーン切り替え実行
        manager->ChangeSceneActual();

        fade->Start(Fade::Status::FadeIn, fade->GetDuration());

        // フェードインへ遷移させる
        manager->ChangeState(std::make_unique<FadeInState>());
    }
}

void FadeOutState::Draw(SceneManager* manager) 
{
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Draw();
    }
    manager->GetFade()->Draw();
}