#pragma once
#include "ISceneTransitionState.h"

namespace FS
{

class NormalState : public ISceneTransitionState
{
public:
    void Update(SceneManager* manager) override;

    void Draw(SceneManager* manager) override;
};

}