#pragma once

#include "Engine.h"
#include "GameObject.h"

class Grid : public GameObject
{
public:
    Grid(Engine* engine);
    ~Grid() override = default;

    void Initialize() override {};
    void Update() override {}
    void Draw() override;

private:
    std::unique_ptr<Model> model_;

};