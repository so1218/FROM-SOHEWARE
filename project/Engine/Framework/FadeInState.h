#pragma once
#include "ISceneTransitionState.h"

namespace FS
{

class FadeInState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}