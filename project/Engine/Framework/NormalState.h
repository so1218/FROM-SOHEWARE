#pragma once
#include "ISceneTransitionState.h"

namespace FE
{

class NormalState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}