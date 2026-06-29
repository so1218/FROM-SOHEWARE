#pragma once
#include "GameObject.h"
#include "Model.h"
#include "Collider.h"
#include "Orb.h"
#include "PropertyBinder.h"

class OrbManager : public FE::GameObject
{
public:
    OrbManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddOrb(); 

private:
    FE::Engine* engine_;
    std::string managerGroupName_;
    std::vector<std::unique_ptr<Orb>> orbs_;

    std::unique_ptr<FE::PropertyBinder> binder_;
    int orbCount_ = 0; // JSONに保存される全体のオーブ数
};