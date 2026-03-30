#pragma once
#include "ISceneTransitionState.h"

namespace FE
{

class FadeOutState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}