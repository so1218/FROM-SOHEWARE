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

    pixelHeight_ = pixelHeight;
    const float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

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
    std::vector<unsigned char> distanceField(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0);
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

        // 輪郭の値を 128 にして、外側 kSdfPadding ピクセルまでの距離を持たせる
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
            LOG_WARN("フォント {} のアトラスが一杯", name);
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

    // RGB = 白, A = 輪郭までの距離
    DirectX::ScratchImage atlasImage;
    if (FAILED(atlasImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, kAtlasSize, kAtlasSize, 1, 1)))
    {
        LOG_ERROR("フォント {} のアトラス画像を作成できなかった", name);
        return false;
    }

    uint8_t* atlasPixels = atlasImage.GetPixels();
    for (size_t pixelIndex = 0; pixelIndex < distanceField.size(); ++pixelIndex)
    {
        atlasPixels[pixelIndex * 4 + 0] = 255;
        atlasPixels[pixelIndex * 4 + 1] = 255;
        atlasPixels[pixelIndex * 4 + 2] = 255;
        atlasPixels[pixelIndex * 4 + 3] = distanceField[pixelIndex];
    }

    textureIndex_ = engine->GetTextureLoader()->LoadFromImage("Font:" + name, atlasImage);

    LOG_INFO("フォント {} を作成 ({} 文字)", name, glyphs_.size());
    return true;
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
