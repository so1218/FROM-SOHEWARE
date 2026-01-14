#pragma once
#include "ISceneTransitionState.h"
#include "FadeOutState.h"

class NormalState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};