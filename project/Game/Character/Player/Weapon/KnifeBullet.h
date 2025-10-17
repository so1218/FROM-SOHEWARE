#pragma once
#include "Bullet.h"
#include "Model.h"

class KnifeBullet : public Bullet 
{
public:
    KnifeBullet(Engine* engine, Camera* camera, const Vector3& pos, const Vector3& dir, int level);

    void Update() override;

    void Draw() override;

    std::vector<std::string> GetGlobalVariableGroupName() const { return { "KnifeBullet" }; }

    Engine* engine_;
    Camera* camera_;
    std::unique_ptr<Model> knifeModel_;
};