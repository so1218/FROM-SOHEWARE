#include "pch.h"
#include "TextureManager.h"
#include "DevError.h"
#include "Engine.h"
#include "AssetFiles.h"

namespace FS
{

namespace fs = std::filesystem;

void TextureManager::LoadAllTextures(Engine* engine)
{
    textures_.clear();
    nameToIndex_.clear();

    // 読み込み中に DDS を作るとフォルダの走査が壊れるので、先に一覧にしてから読む
    const std::vector<fs::path> allFiles = CollectAssetFiles("Textures", { ".png", ".jpg", ".dds", ".tga" });

    for (const auto& filePath : allFiles)
    {
        std::string extLower = filePath.extension().string();
        std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

        std::string stem = filePath.stem().string();
        std::string stemLower = stem;
        std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);

        // 自動生成された .dds と元の .png の二重読み込みを防止
        if (extLower == ".dds")
        {
            if (fs::exists(filePath.parent_path() / (stem + ".png")) ||
                fs::exists(filePath.parent_path() / (stem + ".jpg")) ||
                fs::exists(filePath.parent_path() / (stem + ".tga")))
            {
                continue;
            }
        }

        if (nameToIndex_.count(stemLower) > 0)
        {
            std::cerr << "[Warning] Duplicate texture name (case-insensitive): " << stem << std::endl;
            continue;
        }

        TextureHandleData data;
        data.fullPath = filePath.generic_string();
        data.name = stem;
        data.type = DetectTypeFromPath(filePath);

        // ロード実行 (初回は DDS へ変換、2回目以降は爆速ロード)
        data.handle = engine->GetTextureLoader()->LoadWithAutoDDS(data.fullPath, data.type);

        textures_.push_back(data);
        nameToIndex_[stemLower] = textures_.size() - 1;
    }

    if (nameToIndex_.count("white1x1"))
    {
        errorHandle_ = Get("white1x1");
    }
    else if (!textures_.empty())
    {
        errorHandle_ = textures_[0].handle;
    }
}

uint32_t TextureManager::Get(const std::string& name, std::source_location location)
{
    // 検索時も入力されたキーを小文字化して探す
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    auto it = nameToIndex_.find(nameLower);
    if (it != nameToIndex_.end())
    {
        return textures_[it->second].handle;
    }

    DevError::Report(std::format("テクスチャ \"{}\" が見つからないので、エラー用の画像を使う\nAssets/Textures にその名前のファイルがあるか確認する", name), location);
    return errorHandle_;
}

const TextureHandleData* TextureManager::GetMetaData(const std::string& name)
{
    // メタデータ取得も小文字化検索
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    auto it = nameToIndex_.find(nameLower);
    if (it != nameToIndex_.end())
    {
        return &textures_[it->second];
    }
    return nullptr;
}

TextureType TextureManager::DetectTypeFromPath(const std::filesystem::path& path)
{
    std::string pathStr = path.generic_string();
    std::transform(pathStr.begin(), pathStr.end(), pathStr.begin(), ::tolower);

    if (pathStr.find("/albedo/") != std::string::npos ||
        pathStr.find("/utility/") != std::string::npos)      return TextureType::Albedo;
    if (pathStr.find("/particles/") != std::string::npos) return TextureType::Particle;
    if (pathStr.find("/ui/") != std::string::npos)        return TextureType::UI;
    if (pathStr.find("/environments/") != std::string::npos ||
        pathStr.find("/cubemap/") != std::string::npos)     return TextureType::CubeMap;
    if (pathStr.find("/normal/") != std::string::npos)    return TextureType::Normal;
    if (pathStr.find("/ramps/") != std::string::npos)     return TextureType::Toon;
    if (pathStr.find("/noise/") != std::string::npos)     return TextureType::Noise;

    std::string filename = path.stem().string();
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    if (filename.ends_with("_n") || filename.ends_with("_normal")) return TextureType::Normal;

    return TextureType::Albedo;
}

std::vector<std::string> TextureManager::GetTextureNamesByType(TextureType type) const
{
    std::vector<std::string> names;
    for (const auto& tex : textures_)
    {
        if (tex.type == type)
        {
            names.push_back(tex.name); // オリジナルの名前をリスト化
        }
    }
    return names;
}

}