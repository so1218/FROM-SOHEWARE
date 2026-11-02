#pragma once
#include "EnvironmentProp.h"

// プレハブ単位の共有データ
struct PropGroup
{
    std::string prefabName;
    std::string modelName;
    std::unique_ptr<FS::Model> masterModel;
    std::vector<std::unique_ptr<EnvironmentProp>> instances;
    std::unique_ptr<FS::PropertyBinder> binder;

    // グループ全体のデフォルトコライダー設定
    bool defaultHasCollider = true;
    int defaultColliderType = 0;
};

// マップ上の環境配置物（プロップ）を一括管理するマネージャークラス
class EnvironmentPropManager : public FS::GameObject
{
public:
    EnvironmentPropManager(FS::Engine* engine, const std::string& groupName);
    ~EnvironmentPropManager() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // 指定プレハブのインスタンスを追加
    void AddPropToGroup(const std::string& prefabName);

    // プロップの削除
    void RemoveEnvironmentProp(int targetId);

private:
    // グループの新規作成とモデル読み込み
    void CreateGroup(const std::string& prefabName, const std::string& fallbackModelName);

    // UI操作等によるモデル差し替えの遅延実行
    void ExecutePrefabModelChange();

private:
    FS::Engine* engine_ = nullptr;
    std::string managerGroupName_;
    std::unordered_map<std::string, PropGroup> groups_;

    // モデル遅延変更用
    std::string pendingModelChangePrefab_;
    std::string pendingModelChangeNewName_;

    EnvironmentProp* selectedProp_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;

    int propCount_ = 0;
    int currentTemplateIndex_ = 0;
    int selectedPropIndex_ = -1;
};