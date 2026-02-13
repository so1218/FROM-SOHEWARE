#include "TextureHandleManager.h"
#include "Engine.h"
#include <iostream>

namespace fs = std::filesystem;

void TextureHandleManager::LoadAllTextures(Engine* engine, const std::string& rootDirectory)
{
    textures_.clear();
    nameToIndex_.clear();

    if (!fs::exists(rootDirectory))
    {
        std::cerr << "[TextureHandleManager] Directory not found: " << rootDirectory << std::endl;
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(rootDirectory)) 
    {
        if (entry.is_regular_file()) {
            fs::path filePath = entry.path();

            // 拡張子を取得して小文字化
            std::string ext = filePath.extension().string();
            std::string extLower = ext;
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

            // 画像ファイルのみ対象
            if (extLower == ".png" || extLower == ".jpg" || extLower == ".dds" || extLower == ".tga")
            {

                TextureHandleData data;
                data.fullPath = filePath.generic_string();

                // ファイル名を取得し、小文字に統一して保存
                std::string stem = filePath.stem().string();
                std::string stemLower = stem;
                std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);

                data.name = stem; 

                // タイプ判定
                data.type = DetectTypeFromPath(filePath);

                // ロード
                data.handle = engine->LoadTexture(data.fullPath);

                // 検索用マップには小文字化した名前で登録
                if (nameToIndex_.count(stemLower) > 0) 
                {
                    std::cerr << "[Warning] Duplicate texture name (case-insensitive): " << stem << std::endl;
                    continue;
                }

                textures_.push_back(data);
                nameToIndex_[stemLower] = textures_.size() - 1;
            }
        }
    }

    // エラーハンドルの設定 (小文字で検索)
    if (nameToIndex_.count("white1x1")) 
    {
        errorHandle_ = Get("white1x1");
    }
    else if (!textures_.empty()) 
    {
        errorHandle_ = textures_[0].handle;
    }
}

uint32_t TextureHandleManager::Get(const std::string& name) 
{
    // 検索時も入力されたキーを小文字化して探す
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    auto it = nameToIndex_.find(nameLower);
    if (it != nameToIndex_.end())
    {
        return textures_[it->second].handle;
    }

    std::cerr << "[TextureHandleManager] Texture not found: " << name << std::endl;
    return errorHandle_;
}

const TextureHandleData* TextureHandleManager::GetMetaData(const std::string& name) 
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

TextureType TextureHandleManager::DetectTypeFromPath(const std::filesystem::path& path) 
{
    // パス全体を小文字化して判定 
    std::string pathStr = path.generic_string();
    std::transform(pathStr.begin(), pathStr.end(), pathStr.begin(), ::tolower);

    // 判定ロジック
    if (pathStr.find("/particles/") != std::string::npos) return TextureType::Particle;
    if (pathStr.find("/ui/") != std::string::npos)        return TextureType::UI;
    if (pathStr.find("/environments/") != std::string::npos) return TextureType::CubeMap;
    if (pathStr.find("/normal/") != std::string::npos)    return TextureType::Normal;
    if (pathStr.find("/ramps/") != std::string::npos)     return TextureType::Toon;
    if (pathStr.find("/noise/") != std::string::npos)     return TextureType::Noise;

    // ファイル名ルール
    std::string filename = path.stem().string();
    // ファイル名も小文字化
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

    if (filename.ends_with("_n") || filename.ends_with("_normal")) return TextureType::Normal;
    if (path.extension().string() == ".dds") return TextureType::CubeMap; 

    return TextureType::Albedo;
}