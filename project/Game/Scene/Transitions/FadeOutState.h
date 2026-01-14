#pragma once
#include "ISceneTransitionState.h"

class FadeOutState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};