#pragma once
#include "Structures.h" 

namespace FE
{

class Engine;

class ModelManager
{
public:
    // シングルトン
    static ModelManager& GetInstance()
    {
        static ModelManager instance;
        return instance;
    }

    // コピー禁止
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    // CSVから一括ロード
    void LoadFromCSV(Engine* engine, const std::string& csvPath = "Assets/Data/ModelList.csv");

    // 単体ロード（外部から個別に読み込みたい場合用）
    void Load(const std::string& name, const std::string& path);

    // 名前でモデルデータを取得
    const ModelData* Get(const std::string& name) const;

    // 読み込み済みのモデル名一覧を取得
    std::vector<std::string> GetLoadedModelNames() const;

private:
    ModelManager() = default;
    ~ModelManager() = default;

    // 終了処理
    void Clear();

    Engine* engine_ = nullptr;

    // モデルデータの格納場所
    std::unordered_map<std::string, std::unique_ptr<ModelData>> models_;
};

}