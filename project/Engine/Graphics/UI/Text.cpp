#include "pch.h"
#include "Text.h"
#include "Engine.h"
#include "FontManager.h"
#include "UICommon.h"
#include "Logger.h"

namespace FS
{

namespace
{
    // 縦書きで 90 度回す文字
    bool IsRotatedInVertical(char32_t codepoint)
    {
        static const std::u32string kRotatedCharacters =
            U"ー―—…‥～〜－-_=()[]{}<>（）［］｛｝〈〉《》「」『』【】〔〕";
        return kRotatedCharacters.find(codepoint) != std::u32string::npos;
    }

    // 縦書きで右上に寄せる句読点
    bool IsPunctuationInVertical(char32_t codepoint)
    {
        return codepoint == U'、' || codepoint == U'。' || codepoint == U'，' || codepoint == U'．';
    }

    // 行の先頭に来てはいけない文字
    bool IsLineStartForbidden(char32_t codepoint)
    {
        static const std::u32string kForbiddenCharacters =
            U"、。，．,.!?！？)]}）］｝〉》」』】〕ーぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ";
        return kForbiddenCharacters.find(codepoint) != std::u32string::npos;
    }
}

Text::Text(Engine* engine, const std::string& fontName)
    : engine_(engine)
{
    SetFont(fontName);
}

void Text::SetFont(const std::string& fontName)
{
    FontManager& fontManager = FontManager::GetInstance();
    fontName_ = fontName.empty() ? fontManager.GetDefaultFontName() : fontName;

    font_ = fontManager.Get(fontName_);
    if (font_ == nullptr)
    {
        LOG_WARN("フォント {} が読み込まれていない。既定のフォントを使用", fontName_);
        fontName_ = fontManager.GetDefaultFontName();
        font_ = fontManager.Get(fontName_);
    }
}

void Text::SetText(const std::string& utf8Text)
{
    if (utf8Text == text_)
    {
        return;
    }
    text_ = utf8Text;
    codepoints_ = UI::DecodeUtf8(text_);
}

float Text::GetDisplayScale() const
{
    return fontSize_ / font_->GetPixelHeight();
}


float Text::GetAdvance(char32_t codepoint, const Glyph& glyph) const
{
    // 縦書きの普通の文字は全角 1 文字分ずつ進む
    if (direction_ == TextDirection::Vertical && !IsRotatedInVertical(codepoint))
    {
        return font_->GetEmSize() * GetDisplayScale();
    }
    return glyph.advance * GetDisplayScale();
}

Text::TextLayout Text::BuildLayout() const
{
    TextLayout layout;
    if (font_ == nullptr || codepoints_.empty())
    {
        return layout;
    }

    struct LineCharacter
    {
        char32_t codepoint;
        const Glyph* glyph;
        float advance;
    };
    struct Line
    {
        std::vector<LineCharacter> characters;
        float length = 0.0f;
    };

    // 改行と折り返しで行に分ける
    std::vector<Line> lines(1);
    for (char32_t codepoint : codepoints_)
    {
        if (codepoint == U'\n')
        {
            lines.emplace_back();
            continue;
        }

        const Glyph* glyph = font_->FindGlyph(codepoint);
        if (codepoint == U'\r' || glyph == nullptr)
        {
            continue;
        }

        const float advance = GetAdvance(codepoint, *glyph);
        if (wrapLength_ > 0.0f && !lines.back().characters.empty() &&
            lines.back().length + advance > wrapLength_ && !IsLineStartForbidden(codepoint))
        {
            lines.emplace_back();
        }

        Line& line = lines.back();
        if (!line.characters.empty())
        {
            line.length += characterSpacing_;
        }
        line.characters.push_back({ codepoint, glyph, advance });
        line.length += advance;
    }

    const bool isVertical = (direction_ == TextDirection::Vertical);
    const float displayScale = GetDisplayScale();
    const float lineStep = font_->GetLineHeight() * displayScale * lineSpacing_;

    float maxLineLength = 0.0f;
    for (const Line& line : lines)
    {
        maxLineLength = std::max(maxLineLength, line.length);
    }
    const float linesLength = lineStep * static_cast<float>(lines.size());
    layout.blockSize = isVertical ? Vector2(linesLength, maxLineLength) : Vector2(maxLineLength, linesLength);

    float alignRatio = 0.0f;
    switch (align_)
    {
    case TextAlign::Left:   alignRatio = 0.0f; break;
    case TextAlign::Center: alignRatio = 0.5f; break;
    case TextAlign::Right:  alignRatio = 1.0f; break;
    }

    const float emSize = font_->GetEmSize() * displayScale;
    const float emAscent = font_->GetEmAscent() * displayScale;
    const float ascent = font_->GetAscent() * displayScale;

    for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        const Line& line = lines[lineIndex];
        float pen = (maxLineLength - line.length) * alignRatio;

        // 縦書きの行は右から並ぶ
        const float lineStart = isVertical
            ? layout.blockSize.x - lineStep * static_cast<float>(lineIndex + 1)
            : lineStep * static_cast<float>(lineIndex);
        const float columnCenterX = lineStart + lineStep * 0.5f;

        for (const LineCharacter& character : line.characters)
        {
            const Glyph& glyph = *character.glyph;

            GlyphPlacement placement;
            placement.glyph = character.glyph;
            placement.size = Vector2(glyph.size.x * displayScale, glyph.size.y * displayScale);

            const float offsetX = glyph.offset.x * displayScale;
            // ベースラインから見た文字の上端
            const float topFromBaseline = glyph.offset.y * displayScale - ascent;

            if (!isVertical)
            {
                placement.center = Vector2(
                    pen + offsetX + placement.size.x * 0.5f,
                    lineStart + ascent + topFromBaseline + placement.size.y * 0.5f);
            }
            else if (IsRotatedInVertical(character.codepoint))
            {
                // 横書きの文字枠の中心からのずれを 90 度回して、縦の枠に置く
                const float shiftX = offsetX + placement.size.x * 0.5f - character.advance * 0.5f;
                const float shiftY = emAscent + topFromBaseline + placement.size.y * 0.5f - emSize * 0.5f;

                placement.center = Vector2(columnCenterX - shiftY, pen + character.advance * 0.5f + shiftX);
                placement.rotation = std::numbers::pi_v<float> *0.5f;
            }
            else
            {
                // 全角 1 文字分の枠の中に置く
                float glyphLeft = (emSize - glyph.advance * displayScale) * 0.5f + offsetX;
                float glyphTop = emAscent + topFromBaseline;

                // 句読点は左下から右上へ
                if (IsPunctuationInVertical(character.codepoint))
                {
                    glyphLeft = emSize - glyphLeft - placement.size.x;
                    glyphTop = emSize - glyphTop - placement.size.y;
                }

                placement.center = Vector2(
                    columnCenterX - emSize * 0.5f + glyphLeft + placement.size.x * 0.5f,
                    pen + glyphTop + placement.size.y * 0.5f);
            }

            // 空白は位置だけ進める
            if (placement.size.x > 0.0f && placement.size.y > 0.0f)
            {
                layout.glyphs.push_back(placement);
            }
            pen += character.advance + characterSpacing_;
        }
    }

    return layout;
}

Vector2 Text::MeasureSize() const
{
    return BuildLayout().blockSize;
}

void Text::Draw()
{
    if (!isVisible_ || font_ == nullptr || codepoints_.empty())
    {
        return;
    }

    const TextLayout layout = BuildLayout();
    const Vector2 anchorPosition = UI::ResolveScreenPosition(screenAnchor_, position_);
    const Vector2 blockTopLeft(
        anchorPosition.x - pivot_.x * layout.blockSize.x,
        anchorPosition.y - pivot_.y * layout.blockSize.y);

    // 縁取りを先に全部描く
    if (outlineWidth_ > 0.0f)
    {
        // 画面のピクセルをアトラスのピクセルに直す
        const float outlineAtlasPixels =
            std::min(outlineWidth_ / GetDisplayScale(), static_cast<float>(Font::kSdfPadding - 1));
        SubmitGlyphs(layout, blockTopLeft, outlineColor_, outlineAtlasPixels * Font::kSdfDistancePerPixel);
    }
    SubmitGlyphs(layout, blockTopLeft, color_, 0.0f);
}

void Text::SubmitGlyphs(const TextLayout& layout, const Vector2& blockTopLeft,
    const Vector4& color, float outlineThickness) const
{
    RendererManager* rendererManager = engine_->GetRendererManager();

    for (const GlyphPlacement& placement : layout.glyphs)
    {
        SpriteInstance glyphInstance{};
        glyphInstance.position = Vector2(blockTopLeft.x + placement.center.x, blockTopLeft.y + placement.center.y);
        glyphInstance.size = placement.size;
        glyphInstance.pivot = Vector2(0.5f, 0.5f);
        glyphInstance.rotation = placement.rotation;
        glyphInstance.textureIndex = font_->GetTextureIndex();
        glyphInstance.color = color;
        glyphInstance.uvRect = placement.glyph->uvRect;
        glyphInstance.uvMatrix = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
        glyphInstance.uvTranslate = Vector2(0.0f, 0.0f);
        glyphInstance.emissiveIntensity = 1.0f;
        glyphInstance.drawMode = SPRITE_DRAW_MODE_SDF_TEXT;
        glyphInstance.outlineColor = color;
        glyphInstance.outlineThickness = outlineThickness;

        rendererManager->SubmitSprite(glyphInstance, layerOrder_);
    }
}

}
