#pragma once
#include "ISceneTransitionState.h"

namespace FE
{

class FadeInState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}