#pragma once
#include "EnvironmentProp.h"

// 同一プレハブの共有データを定義する構造体
struct PropGroup 
{
    std::string prefabName;
    std::string modelName;
    std::unique_ptr<FE::Model> masterModel; 
    std::vector<std::unique_ptr<EnvironmentProp>> instances; 
    std::unique_ptr<FE::PropertyBinder> binder;

    // グループ全体で共有する基本設定
    bool defaultHasCollider = true;
    int defaultColliderType = 0;
};

// マップ上の環境配置物（プロップ）を一括管理するマネージャークラス
class EnvironmentPropManager : public FE::GameObject
{
public:
    EnvironmentPropManager(FE::Engine* engine, const std::string& groupName);
    ~EnvironmentPropManager() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    // 特定のプレハブグループへプロップインスタンスを追加
    void AddPropToGroup(const std::string& prefabName);

    // 指定IDのプロップ削除および後続IDの詰め処理
    void RemoveEnvironmentProp(int targetId);

private:
    // プレハブグループの動的生成およびマスターモデルの構築
    void CreateGroup(const std::string& prefabName, const std::string& fallbackModelName);

    // UI非同期操作によるモデルリロードの遅延適用処理
    void ExecutePrefabModelChange();

private:
    FE::Engine* engine_ = nullptr;
    std::string managerGroupName_;
    std::unordered_map<std::string, PropGroup> groups_;

    // 遅延適用用パラメータ
    std::string pendingModelChangePrefab_;
    std::string pendingModelChangeNewName_;

    EnvironmentProp* selectedProp_ = nullptr;
    std::unique_ptr<FE::PropertyBinder> binder_;

    int propCount_ = 0;
    int currentTemplateIndex_ = 0;
    int selectedPropIndex_ = -1;
};