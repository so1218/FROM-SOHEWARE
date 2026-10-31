#include "pch.h"
#include "Font.h"
#include "Engine.h"
#include "TextureLoader.h"
#include "Logger.h"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "imstb_rectpack.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"

namespace FS
{

bool Font::Load(Engine* engine, const std::string& name, const std::string& fontPath,
    float pixelHeight, const std::u32string& characters)
{
    pixelHeight_ = pixelHeight;

    // フォントファイル・大きさ・文字のどれかが変わったら作り直す
    std::error_code errorCode;
    const auto fontFileSize = std::filesystem::file_size(fontPath, errorCode);
    const auto fontWriteTime = std::filesystem::last_write_time(fontPath, errorCode).time_since_epoch().count();
    std::string cacheKeySource = std::format("{}|{}|{}|{}|{}|{}|{}|",
        fontPath, fontFileSize, fontWriteTime, pixelHeight, kSdfPadding, kAtlasSize, kCacheVersion);
    cacheKeySource.append(reinterpret_cast<const char*>(characters.data()), characters.size() * sizeof(char32_t));
    const uint64_t cacheKey = std::hash<std::string>{}(cacheKeySource);

    const std::string cachePath = "Cache/Fonts/" + name + ".fontcache";
    std::vector<unsigned char> distanceField;
    if (!LoadCache(cachePath, cacheKey, distanceField))
    {
        if (!BuildAtlas(fontPath, characters, distanceField))
        {
            return false;
        }
        SaveCache(cachePath, cacheKey, distanceField);
    }

    // A = 輪郭までの距離 (シェーダーは a だけを読む)
    DirectX::ScratchImage atlasImage;
    if (FAILED(atlasImage.Initialize2D(DXGI_FORMAT_A8_UNORM, kAtlasSize, kAtlasSize, 1, 1)))
    {
        LOG_ERROR("フォント {} のアトラス画像を作成出来なかった", name);
        return false;
    }
    std::memcpy(atlasImage.GetPixels(), distanceField.data(), distanceField.size());

    textureIndex_ = engine->GetTextureLoader()->LoadFromImage("Font:" + name, atlasImage);

    LOG_INFO("フォント {} を作成 ({} 文字)", name, glyphs_.size());
    return true;
}

bool Font::BuildAtlas(const std::string& fontPath, const std::u32string& characters,
    std::vector<unsigned char>& distanceField)
{
    std::ifstream fontFile(fontPath, std::ios::binary);
    if (!fontFile)
    {
        LOG_ERROR("フォントファイルが見つからない: {}", fontPath);
        return false;
    }
    std::vector<unsigned char> fontData(
        (std::istreambuf_iterator<char>(fontFile)), std::istreambuf_iterator<char>());

    stbtt_fontinfo fontInfo{};
    if (!stbtt_InitFont(&fontInfo, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0)))
    {
        LOG_ERROR("フォントの読み込みに失敗: {}", fontPath);
        return false;
    }

    const float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight_);

    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    ascent_ = ascent * fontScale;
    lineHeight_ = (ascent - descent + lineGap) * fontScale;
    emSize_ = fontScale / stbtt_ScaleForMappingEmToPixels(&fontInfo, 1.0f);

    int typoAscent = 0;
    int typoDescent = 0;
    int typoLineGap = 0;
    if (stbtt_GetFontVMetricsOS2(&fontInfo, &typoAscent, &typoDescent, &typoLineGap))
    {
        emAscent_ = typoAscent * fontScale;
    }
    else
    {
        emAscent_ = emSize_ * 0.88f;
    }

    // 左から順に詰めていく
    distanceField.assign(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0);
    int penX = kGlyphSpacing;
    int penY = kGlyphSpacing;
    int rowHeight = 0;

    for (char32_t codepoint : characters)
    {
        if (glyphs_.contains(codepoint) || stbtt_FindGlyphIndex(&fontInfo, static_cast<int>(codepoint)) == 0)
        {
            continue;
        }

        int advanceWidth = 0;
        stbtt_GetCodepointHMetrics(&fontInfo, static_cast<int>(codepoint), &advanceWidth, nullptr);

        Glyph glyph;
        glyph.advance = advanceWidth * fontScale;

        // 輪郭の値を 128 (≒ 0.5) にして、外側 kSdfPadding ピクセルまでの距離を持たせる
        int glyphWidth = 0;
        int glyphHeight = 0;
        int glyphLeft = 0;
        int glyphTop = 0;
        unsigned char* sdfBitmap = stbtt_GetCodepointSDF(&fontInfo, fontScale, static_cast<int>(codepoint),
            kSdfPadding, 128, 128.0f / kSdfPadding,
            &glyphWidth, &glyphHeight, &glyphLeft, &glyphTop);

        // 空白など、形のない文字
        if (sdfBitmap == nullptr)
        {
            glyphs_[codepoint] = glyph;
            continue;
        }

        if (penX + glyphWidth + kGlyphSpacing > kAtlasSize)
        {
            penX = kGlyphSpacing;
            penY += rowHeight + kGlyphSpacing;
            rowHeight = 0;
        }
        if (penY + glyphHeight + kGlyphSpacing > kAtlasSize)
        {
            stbtt_FreeSDF(sdfBitmap, nullptr);
            LOG_WARN("フォント {} のアトラスが一杯", fontPath);
            break;
        }

        for (int row = 0; row < glyphHeight; ++row)
        {
            std::memcpy(&distanceField[static_cast<size_t>(penY + row) * kAtlasSize + penX],
                &sdfBitmap[static_cast<size_t>(row) * glyphWidth], glyphWidth);
        }
        stbtt_FreeSDF(sdfBitmap, nullptr);

        glyph.uvRect = Vector4(
            static_cast<float>(penX) / kAtlasSize,
            static_cast<float>(penY) / kAtlasSize,
            static_cast<float>(penX + glyphWidth) / kAtlasSize,
            static_cast<float>(penY + glyphHeight) / kAtlasSize);
        glyph.size = Vector2(static_cast<float>(glyphWidth), static_cast<float>(glyphHeight));
        glyph.offset = Vector2(static_cast<float>(glyphLeft), ascent_ + static_cast<float>(glyphTop));
        glyphs_[codepoint] = glyph;

        penX += glyphWidth + kGlyphSpacing;
        rowHeight = std::max(rowHeight, glyphHeight);
    }

    return true;
}

bool Font::LoadCache(const std::string& cachePath, uint64_t cacheKey, std::vector<unsigned char>& distanceField)
{
    std::ifstream cacheFile(cachePath, std::ios::binary);
    if (!cacheFile)
    {
        return false;
    }

    uint64_t savedKey = 0;
    cacheFile.read(reinterpret_cast<char*>(&savedKey), sizeof(savedKey));
    if (savedKey != cacheKey)
    {
        return false;
    }

    float metrics[4] = {};
    uint32_t glyphCount = 0;
    uint32_t usedHeight = 0;
    cacheFile.read(reinterpret_cast<char*>(metrics), sizeof(metrics));
    cacheFile.read(reinterpret_cast<char*>(&glyphCount), sizeof(glyphCount));
    cacheFile.read(reinterpret_cast<char*>(&usedHeight), sizeof(usedHeight));
    if (!cacheFile || usedHeight > kAtlasSize)
    {
        return false;
    }

    lineHeight_ = metrics[0];
    ascent_ = metrics[1];
    emSize_ = metrics[2];
    emAscent_ = metrics[3];

    glyphs_.clear();
    glyphs_.reserve(glyphCount);
    for (uint32_t glyphIndex = 0; glyphIndex < glyphCount; ++glyphIndex)
    {
        char32_t codepoint = 0;
        float values[9] = {};
        cacheFile.read(reinterpret_cast<char*>(&codepoint), sizeof(codepoint));
        cacheFile.read(reinterpret_cast<char*>(values), sizeof(values));

        Glyph glyph;
        glyph.uvRect = Vector4(values[0], values[1], values[2], values[3]);
        glyph.size = Vector2(values[4], values[5]);
        glyph.offset = Vector2(values[6], values[7]);
        glyph.advance = values[8];
        glyphs_[codepoint] = glyph;
    }

    // 使っている段だけ保存してあるので、残りは 0 のまま
    distanceField.assign(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0);
    cacheFile.read(reinterpret_cast<char*>(distanceField.data()), static_cast<std::streamsize>(usedHeight) * kAtlasSize);

    if (!cacheFile)
    {
        glyphs_.clear();
        return false;
    }
    return true;
}

void Font::SaveCache(const std::string& cachePath, uint64_t cacheKey, const std::vector<unsigned char>& distanceField) const
{
    std::filesystem::create_directories(std::filesystem::path(cachePath).parent_path());
    std::ofstream cacheFile(cachePath, std::ios::binary);
    if (!cacheFile)
    {
        LOG_WARN("フォントのキャッシュを保存できなかった: {}", cachePath);
        return;
    }

    uint32_t usedHeight = 0;
    for (const auto& [codepoint, glyph] : glyphs_)
    {
        usedHeight = std::max(usedHeight, static_cast<uint32_t>(std::ceil(glyph.uvRect.w * kAtlasSize)));
    }

    const float metrics[4] = { lineHeight_, ascent_, emSize_, emAscent_ };
    const uint32_t glyphCount = static_cast<uint32_t>(glyphs_.size());
    cacheFile.write(reinterpret_cast<const char*>(&cacheKey), sizeof(cacheKey));
    cacheFile.write(reinterpret_cast<const char*>(metrics), sizeof(metrics));
    cacheFile.write(reinterpret_cast<const char*>(&glyphCount), sizeof(glyphCount));
    cacheFile.write(reinterpret_cast<const char*>(&usedHeight), sizeof(usedHeight));

    for (const auto& [codepoint, glyph] : glyphs_)
    {
        const float values[9] =
        {
            glyph.uvRect.x, glyph.uvRect.y, glyph.uvRect.z, glyph.uvRect.w,
            glyph.size.x, glyph.size.y, glyph.offset.x, glyph.offset.y, glyph.advance,
        };
        cacheFile.write(reinterpret_cast<const char*>(&codepoint), sizeof(codepoint));
        cacheFile.write(reinterpret_cast<const char*>(values), sizeof(values));
    }

    cacheFile.write(reinterpret_cast<const char*>(distanceField.data()), static_cast<std::streamsize>(usedHeight) * kAtlasSize);
}

const Glyph* Font::FindGlyph(char32_t codepoint) const
{
    auto foundGlyph = glyphs_.find(codepoint);
    if (foundGlyph != glyphs_.end())
    {
        return &foundGlyph->second;
    }

    auto fallbackGlyph = glyphs_.find(U'?');
    return (fallbackGlyph != glyphs_.end()) ? &fallbackGlyph->second : nullptr;
}

}
