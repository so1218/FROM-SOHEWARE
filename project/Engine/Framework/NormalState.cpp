#include "pch.h"
#include "NormalState.h"
#include "FadeOutState.h"
#include "BaseScene.h"
#include "Fade.h"

namespace FE
{

void NormalState::Update(SceneManager* manager)
{
    // 現在のシーンを更新
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Update();
    }

    // 次のシーンへの切り替えリクエストが来ているかチェック
    if (manager->HasNextSceneID())
    {
        // フェードを開始設定
        manager->GetFade()->Start(Fade::Status::FadeOut, manager->GetFade()->GetDuration());

        // 状態をフェードアウトへ切り替え
        manager->ChangeState(std::make_unique<FadeOutState>());
    }
}

void NormalState::Draw(SceneManager* manager)
{
    if (manager->GetCurrentScene())
    {
        manager->GetCurrentScene()->Draw();
    }
}

}