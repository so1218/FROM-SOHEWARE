#include "FadeInState.h"

#include <memory> 

void FadeInState::Update(SceneManager* manager)
{
    // シーンとフェードの更新
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Update();
    }

    auto* fade = manager->GetFade();
    fade->Update();

    // フェードインが終わったかチェック
    if (fade->IsFinished())
    {
        fade->Stop();

        // 状態を通常へ戻す
        manager->ChangeState(std::make_unique<NormalState>());
    }
}

void FadeInState::Draw(SceneManager* manager)
{
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Draw();
    }
    manager->GetFade()->Draw();
}