#include "pch.h"
#include "ModelManager.h"
#include "ModelLoader.h"
#include "StringUtils.h" 
#include "TextureManager.h" 
#include "Engine.h" 

namespace FE
{

// CSV読み込み
void ModelManager::LoadFromCSV(Engine* engine, const std::string& csvPath)
{
    engine_ = engine;

    std::ifstream file(csvPath);
    if (!file.is_open())
    {
        std::cerr << "[ModelManager] CSV not found: " << csvPath << std::endl;
        assert(false && "ModelList.csv not found.");
        return;
    }

    // 共通の親フォルダパス
    const std::string kDirectoryPath = "Assets/Models/";

    std::string line;
    // ヘッダ行がある場合はスキップ
    std::getline(file, line);

    while (std::getline(file, line))
    {
        // 空行やコメントのスキップ処理
        if (line.empty()) continue;
        size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar == std::string::npos) continue;
        if (line[firstChar] == '#' || (line.size() > firstChar + 1 && line[firstChar] == '/' && line[firstChar + 1] == '/')) 
        {
            continue;
        }

        std::istringstream stream(line);
        std::string name, path;

        if (std::getline(stream, name, ',') && std::getline(stream, path))
        {
            name = StringUtils::Trim(name);
            path = StringUtils::Trim(path);

            if (!name.empty() && !path.empty())
            {
                std::string fullPath = kDirectoryPath + path;

                // 自分自身のLoad関数を呼び出す
                Load(name, fullPath);
            }
        }
    }
}

void ModelManager::Load(const std::string& name, const std::string& path)
{
    // 重複チェック
    if (models_.find(name) != models_.end())
    {
        return; 
    }

    ModelLoader loader;

    // ModelLoaderの実装に合わせて呼び出し
    ModelData data = loader.LoadModel(path.c_str());

    // ロードした直後に、1回だけマテリアルを作ってキャッシュしておく
    auto& texManager = TextureManager::GetInstance();
    for (size_t i = 0; i < data.meshes.size(); ++i)
    {
        // 1回だけ作成
        MaterialHandle defMat = engine_->GetMaterialManager()->CreateMaterial(engine_->GetGraphicsDevice()->GetDevice());

        defMat.textureHandle = texManager.Get("white1x1");
        defMat.envMapHandle = texManager.Get("skybox");
        defMat.toonRampHandle = texManager.Get("toonRamp_01");
        defMat.dissolveMapHandle = texManager.Get("white1x1");
        defMat.normalMapHandle = texManager.Get("white1x1");

        if (defMat.materialData) {
            defMat.materialData->uvTransform = defMat.uvTransformData.matWorld_;
        }

        data.defaultMaterials.push_back(defMat);
    }

    // 所有権をunique_ptrに移譲して登録
    models_[name] = std::make_unique<ModelData>(std::move(data));
}

const ModelData* ModelManager::Get(const std::string& name) const
{
    auto it = models_.find(name);
    if (it == models_.end())
    {
        std::cerr << "[ModelManager] Model not found: " << name << std::endl;
        return nullptr; 
    }
    return it->second.get();
}

std::vector<std::string> ModelManager::GetLoadedModelNames() const
{
    std::vector<std::string> names;
    // unordered_mapからキー（名前）だけを抽出
    for (const auto& pair : models_)
    {
        names.push_back(pair.first);
    }
    return names;
}

void ModelManager::Clear()
{
    models_.clear();
}

}