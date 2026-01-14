#pragma once
#include "ISceneTransitionState.h"
#include "NormalState.h"

class FadeInState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};