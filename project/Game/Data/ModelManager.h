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

    // 初期化・終了
    void Initialize();
    void Finalize();

    // 名前とパスでロード
    void Load(const std::string& name, const std::string& path);

    // 名前でモデルデータを取得
    const ModelData* GetModel(const std::string& name) const;

private:
    ModelManager() = default;
    ~ModelManager() = default;

    // モデルデータの格納場所
    std::unordered_map<std::string, std::unique_ptr<ModelData>> models_;

    // エラー時に返す用のダミーモデル
    std::unique_ptr<ModelData> errorModel_;
};