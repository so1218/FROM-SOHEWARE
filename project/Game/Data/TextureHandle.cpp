#include "TextureHandle.h"

#include <cassert>
#include <filesystem>
#include <algorithm>

std::array<uint32_t, TEXTURES_COUNT> TextureHandle::textureHandles_{};
bool TextureHandle::initialized_ = false;

std::array<TextureType, TEXTURES_COUNT> TextureHandle::textureTypes_{};

constexpr std::array<TextureDefinition, TEXTURES_COUNT> TextureHandle::textureDefinitions_;

void TextureHandle::Initialize(Engine* engine)
{
    if (initialized_) return;

    for (const auto& def : textureDefinitions_)
    {
        // パス情報を分解して取得
        std::filesystem::path filePath(def.path);

        // 拡張子を取得
        std::string extension = filePath.extension().string();

        // ファイル名（拡張子なし）を取得 
        std::string filename = filePath.stem().string();

        // 親フォルダのパスを取得
        std::string folderPath = filePath.parent_path().string();

        // 判定用にすべて小文字に変換
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        std::transform(folderPath.begin(), folderPath.end(), folderPath.begin(), ::tolower);

        // キューブマップ
        // 拡張子が".dds"
        if (extension == ".dds")
        {
            textureTypes_[def.id] = TextureType::CubeMap;
        }

        // ノーマルマップ
        else if (filename.ends_with("normal_") || folderPath.find("normal") != std::string::npos)
        {
            textureTypes_[def.id] = TextureType::Normal;
        }

        // トゥーンランプ
        else if (folderPath.find("ramps") != std::string::npos || filename.find("ramp") != std::string::npos)
        {
            textureTypes_[def.id] = TextureType::Toon;
        }

        // ノイズ
        else if (folderPath.find("noise") != std::string::npos)
        {
            textureTypes_[def.id] = TextureType::Noise;
        }

        // それ以外はすべて通常の画像
        else
        {
            textureTypes_[def.id] = TextureType::Albedo;
        }

        // ロード処理
        textureHandles_[def.id] = engine->LoadTexture(def.path);
    }

    initialized_ = true;
}

uint32_t TextureHandle::Get(TextureID id)
{
    assert(initialized_ && "TextureHandle::Initialize must be called before Get()");
    assert(id >= 0 && id < TEXTURES_COUNT);
    return textureHandles_[id];
}

TextureType TextureHandle::GetType(TextureID id)
{
    assert(id >= 0 && id < TEXTURES_COUNT);

    return textureTypes_[id];
}

std::string TextureHandle::GetFileName(TextureID id)
{
    // 初期化チェックや範囲チェック
    assert(id >= 0 && id < TEXTURES_COUNT);

    // フルパスを取得
    std::string fullPath = textureDefinitions_[id].path;

    // filesystemを使ってファイル名だけ抽出
    return std::filesystem::path(fullPath).filename().string();
}