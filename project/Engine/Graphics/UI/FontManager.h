#pragma once
#include "Font.h"

namespace FS
{

class Engine;

class FontManager
{
public:
    static FontManager& GetInstance()
    {
        static FontManager instance;
        return instance;
    }

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // FontList.csv のフォントをすべて読み込む
    void LoadFromCSV(Engine* engine, const std::string& csvPath = "Assets/Data/FontList.csv");

    bool Load(Engine* engine, const std::string& name, const std::string& fontPath,
        float pixelHeight, const std::string& charsetPath);

    const Font* Get(const std::string& name) const;

    // 読み込んだ順
    const std::vector<std::string>& GetFontNames() const { return fontNames_; }
    std::string GetDefaultFontName() const { return fontNames_.empty() ? "" : fontNames_.front(); }

    void Clear()
    {
        fonts_.clear();
        fontNames_.clear();
    }

private:
    FontManager() = default;
    ~FontManager() = default;

    static std::u32string BuildCharacterList(const std::string& charsetPath);

    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
    std::vector<std::string> fontNames_;
};

}
