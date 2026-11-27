#pragma once
#include "EnvironmentProp.h"

// 同じプレハブのプロップで共有するもの
struct PropGroup
{
    std::string prefabName;
    std::string modelName;
    std::unique_ptr<FS::Model> masterModel;
    std::vector<std::unique_ptr<EnvironmentProp>> instances;
    std::unique_ptr<FS::PropertyBinder> binder;

    // このプレハブの当たり判定の初期設定
    bool defaultHasCollider = true;
    int defaultColliderType = 0;
};

// ステージのプロップをまとめて持ち、配置を編集する
class EnvironmentPropManager : public FS::GameObject
{
public:
    EnvironmentPropManager(FS::Engine* engine, const std::string& groupName);
    ~EnvironmentPropManager() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    // プレハブのプロップを 1 つ足す
    void AddPropToGroup(const std::string& prefabName);

    // 番号を指定してプロップを消す
    void RemoveEnvironmentProp(int targetId);

    // 拾って消えたプロップを、置いたときの状態に戻す
    void ResetProps();

private:
    // プレハブのグループを作り、モデルを読み込む
    void CreateGroup(const std::string& prefabName, const std::string& fallbackModelName);

    // ImGui で選んだモデルへの差し替えを、次のフレームの頭で行う
    void ExecutePrefabModelChange();

private:
    FS::Engine* engine_ = nullptr;
    std::string managerGroupName_;
    std::unordered_map<std::string, PropGroup> groups_;

    // 次のフレームで差し替えるモデル
    std::string pendingModelChangePrefab_;
    std::string pendingModelChangeNewName_;

    EnvironmentProp* selectedProp_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    int propCount_ = 0;
    int currentTemplateIndex_ = 0;
    int selectedPropIndex_ = -1;
};