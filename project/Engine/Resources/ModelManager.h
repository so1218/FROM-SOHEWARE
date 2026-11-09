#pragma once
#include "Structures.h" 

namespace FS
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

    // フォルダ内のモデルをすべて読み込む。名前は拡張子を除いたファイル名
    // Engine/Assets/Models と Assets/Models のモデルをすべて読む
    void LoadAllModels(Engine* engine);

    // 単体ロード（外部から個別に読み込みたい場合用）
    void Load(const std::string& name, const std::string& path);

    // 名前でモデルデータを取得
    const ModelData* Get(const std::string& name, std::source_location location = std::source_location::current()) const;

    // 読み込み済みのモデル名一覧を取得
    std::vector<std::string> GetLoadedModelNames() const;

    // 終了処理
    void Clear();

private:
    ModelManager() = default;
    ~ModelManager() = default;

    Engine* engine_ = nullptr;

    // モデルデータの格納場所
    std::unordered_map<std::string, std::unique_ptr<ModelData>> models_;
};

}