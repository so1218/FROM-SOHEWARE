#include "pch.h"
#include "TextureManager.h"
#include "Engine.h"

namespace FS
{

namespace fs = std::filesystem;

void TextureManager::LoadAllTextures(Engine* engine, const std::string& rootDirectory)
{
    textures_.clear();
    nameToIndex_.clear();

    if (!fs::exists(rootDirectory))
    {
        std::cerr << "[TextureManager] Directory not found: " << rootDirectory << std::endl;
        return;
    }

    // ディレクトリ走査中に DDS ファイルを新規生成するとイテレータが破綻するため、全ファイルパスを配列に収集
    std::vector<fs::path> allFiles;
    for (const auto& entry : fs::recursive_directory_iterator(rootDirectory))
    {
        if (entry.is_regular_file()) {
            allFiles.push_back(entry.path());
        }
    }

    // 収集したリストをもとに処理を行う
    for (const auto& filePath : allFiles)
    {
        std::string ext = filePath.extension().string();
        std::string extLower = ext;
        std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

        if (extLower == ".png" || extLower == ".jpg" || extLower == ".dds" || extLower == ".tga")
        {
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

            TextureHandleData data;
            data.fullPath = filePath.generic_string();
            data.name = stem;
            data.type = DetectTypeFromPath(filePath);

            // ロード実行 (初回は DDS へ変換、2回目以降は爆速ロード)
            data.handle = engine->GetTextureLoader()->LoadWithAutoDDS(data.fullPath, data.type);

            if (nameToIndex_.count(stemLower) > 0)
            {
                std::cerr << "[Warning] Duplicate texture name (case-insensitive): " << stem << std::endl;
                continue;
            }

            textures_.push_back(data);
            nameToIndex_[stemLower] = textures_.size() - 1;
        }
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

uint32_t TextureManager::Get(const std::string& name)
{
    // 検索時も入力されたキーを小文字化して探す
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    auto it = nameToIndex_.find(nameLower);
    if (it != nameToIndex_.end())
    {
        return textures_[it->second].handle;
    }

    std::cerr << "[TextureManager] Texture not found: " << name << std::endl;
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
    if (pathStr.find("/luts/") != std::string::npos)      return TextureType::LUT;

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