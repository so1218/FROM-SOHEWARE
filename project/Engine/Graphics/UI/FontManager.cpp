#include "pch.h"
#include "FontManager.h"
#include "UICommon.h"
#include "Logger.h"
#include "StringUtils.h"

namespace FS
{

void FontManager::LoadFromCSV(Engine* engine, const std::string& csvPath)
{
    std::ifstream csvFile(csvPath);
    if (!csvFile)
    {
        LOG_ERROR("フォント一覧が見つからない: {}", csvPath);
        return;
    }

    const std::string fontDirectory = "Assets/Data/Fonts/";

    std::string line;
    std::getline(csvFile, line);   // ヘッダー

    while (std::getline(csvFile, line))
    {
        line = StringUtils::Trim(line);
        if (line.empty() || line.starts_with("#"))
        {
            continue;
        }

        // Name,Path,PixelHeight,Charset
        std::vector<std::string> columns;
        std::istringstream lineStream(line);
        std::string column;
        while (std::getline(lineStream, column, ','))
        {
            columns.push_back(StringUtils::Trim(column));
        }
        if (columns.size() < 3)
        {
            LOG_WARN("FontList.csv の書式が正しくない: {}", line);
            continue;
        }

        std::string charsetFileName = "Charset.txt";
        if (columns.size() >= 4 && !columns[3].empty())
        {
            charsetFileName = columns[3];
        }

        Load(engine, columns[0], fontDirectory + columns[1], std::stof(columns[2]), fontDirectory + charsetFileName);
    }
}

bool FontManager::Load(Engine* engine, const std::string& name, const std::string& fontPath,
    float pixelHeight, const std::string& charsetPath)
{
    if (fonts_.contains(name))
    {
        return true;
    }

    auto font = std::make_unique<Font>();
    if (!font->Load(engine, name, fontPath, pixelHeight, BuildCharacterList(charsetPath)))
    {
        return false;
    }

    fonts_[name] = std::move(font);
    fontNames_.push_back(name);
    return true;
}

const Font* FontManager::Get(const std::string& name) const
{
    auto foundFont = fonts_.find(name);
    return (foundFont != fonts_.end()) ? foundFont->second.get() : nullptr;
}

std::u32string FontManager::BuildCharacterList(const std::string& charsetPath)
{
    std::u32string characters;

    auto appendRange = [&characters](char32_t first, char32_t last)
        {
            for (char32_t codepoint = first; codepoint <= last; ++codepoint)
            {
                characters.push_back(codepoint);
            }
        };

    appendRange(0x0020, 0x007E);   // ASCII
    appendRange(0x3000, 0x303F);   // 記号
    appendRange(0x3040, 0x309F);   // ひらがな
    appendRange(0x30A0, 0x30FF);   // カタカナ
    appendRange(0xFF01, 0xFF5E);   // 全角英数字

    // 漢字は Charset.txt から
    std::ifstream charsetFile(charsetPath, std::ios::binary);
    if (!charsetFile)
    {
        LOG_WARN("文字一覧ファイルが見つからない: {}", charsetPath);
        return characters;
    }

    std::string charsetText((std::istreambuf_iterator<char>(charsetFile)), std::istreambuf_iterator<char>());
    for (char32_t codepoint : UI::DecodeUtf8(charsetText))
    {
        // 改行などの制御文字と BOM は飛ばす
        if (codepoint < U' ' || codepoint == 0xFEFF)
        {
            continue;
        }
        characters.push_back(codepoint);
    }
    return characters;
}

}
