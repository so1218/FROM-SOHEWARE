#pragma once
#include "EnvironmentProp.h"

class EnvironmentPropManager : public FE::GameObject
{
public:
    EnvironmentPropManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void AddProp();
    void RemoveEnvironmentProp(int index);

private:
    FE::Engine* engine_;
    std::string managerGroupName_;
    std::vector<std::unique_ptr<EnvironmentProp>> props_;

    std::unique_ptr<FE::PropertyBinder> binder_;
    int propCount_ = 0; // JSONに保存される全体の数

    // テンプレート用（最後に設定をコピーした元のインデックス）
    int currentTemplateIndex_ = 0;

    int selectedPropIndex_ = -1;
};