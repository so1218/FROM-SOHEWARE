#pragma once
#include "SceneManager.h"

// 遷移状態の基底クラス
class ISceneTransitionState
{
public:
    virtual ~ISceneTransitionState() = default;
    virtual void Update(SceneManager* manager) = 0;
    virtual void Draw(SceneManager* manager) = 0;
};