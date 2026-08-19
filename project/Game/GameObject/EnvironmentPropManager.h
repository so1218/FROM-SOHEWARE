#pragma once
#include "EnvironmentProp.h"

struct PropGroup 
{
    std::string prefabName;
    std::string modelName;
    std::unique_ptr<FE::Model> masterModel; // このグループのマスターモデル
    std::vector<std::unique_ptr<EnvironmentProp>> instances; // 実体のリスト
    std::unique_ptr<FE::PropertyBinder> binder;

    // グループ全体で共有する基本設定
    bool defaultHasCollider = true;
    int defaultColliderType = 0;
};

class EnvironmentPropManager : public FE::GameObject
{
public:
    EnvironmentPropManager(FE::Engine* engine, const std::string& groupName);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void RemoveEnvironmentProp(int index);

    // 特定のグループ（モデル）に新しいプロップを追加
    void AddPropToGroup(const std::string& modelName);

private:
    FE::Engine* engine_;
    std::string managerGroupName_;
    std::unordered_map<std::string, PropGroup> groups_;

    // 新しい Prefab を作る関数
    void CreateGroup(const std::string& prefabName, const std::string& fallbackModelName);

    // モデル差し替えの遅延実行用
    std::string pendingModelChangePrefab_ = "";
    std::string pendingModelChangeNewName_ = "";

    // 実際の差し替え処理を行う関数
    void ExecutePrefabModelChange();

    // インデックス(int)ではなく、ポインタで選択中のプロップを管理する
    EnvironmentProp* selectedProp_ = nullptr;

    std::unique_ptr<FE::PropertyBinder> binder_;
    int propCount_ = 0; // JSONに保存される全体の数

    // テンプレート用（最後に設定をコピーした元のインデックス）
    int currentTemplateIndex_ = 0;

    int selectedPropIndex_ = -1;
};