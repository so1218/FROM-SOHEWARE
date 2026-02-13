#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <cassert>
#include "Structures.h" 

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
    void LoadFromCSV(const std::string& csvPath = "Assets/Data/ModelList.csv");

    // 単体ロード（外部から個別に読み込みたい場合用）
    void Load(const std::string& name, const std::string& path);

    // 名前でモデルデータを取得
    const ModelData* GetModel(const std::string& name) const;

private:
    ModelManager() = default;
    ~ModelManager() = default;

    // 終了処理
    void Clear();

    // モデルデータの格納場所
    std::unordered_map<std::string, std::unique_ptr<ModelData>> models_;
};