#pragma once
#include "ISceneTransitionState.h"
#include "NormalState.h"

namespace FE
{

class FadeInState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}