#pragma once
#include "Vector2.h"
#include "Vector4.h"
#include "UIAnchor.h"

namespace FS
{

class Engine;
class Font;
struct Glyph;

// 行の揃え。縦書きでは Left = 上揃え, Right = 下揃え
enum class TextAlign : int32_t
{
    Left = 0,
    Center = 1,
    Right = 2,
};

enum class TextDirection : int32_t
{
    Horizontal = 0,   // 横書き
    Vertical = 1,     // 縦書き (行は右から左へ並ぶ)
};

class Text
{
public:
    // fontName を省略すると FontList.csv の先頭のフォントを使う
    Text(Engine* engine, const std::string& fontName = "");

    void Draw();

    // UTF-8
    void SetText(const std::string& utf8Text);
    const std::string& GetText() const { return text_; }

    void SetFont(const std::string& fontName);
    const std::string& GetFontName() const { return fontName_; }

    void SetPosition(const Vector2& position) { position_ = position; }
    void SetScreenAnchor(const Vector2& screenAnchor) { screenAnchor_ = screenAnchor; }
    void SetPivot(const Vector2& pivot) { pivot_ = pivot; }
    void SetAlign(TextAlign align) { align_ = align; }
    void SetDirection(TextDirection direction) { direction_ = direction; }

    // 画面基準と文章の基準点を同じ位置にそろえる (右上なら画面の右上に文章の右上を合わせる)
    void SetAnchor(UIAnchor anchor)
    {
        screenAnchor_ = ToAnchorVector(anchor);
        pivot_ = screenAnchor_;
    }

    void SetFontSize(float fontSize) { fontSize_ = fontSize; }
    void SetLineSpacing(float lineSpacing) { lineSpacing_ = lineSpacing; }
    // 文字と文字の間に足すすき間 (ピクセル)。マイナスで詰める
    void SetCharacterSpacing(float characterSpacing) { characterSpacing_ = characterSpacing; }
    // 1 行の長さがこれを超えたら折り返す (ピクセル)。0 なら折り返さない
    void SetWrapLength(float wrapLength) { wrapLength_ = wrapLength; }
    void SetColor(const Vector4& color) { color_ = color; }
    void SetOutline(float width, const Vector4& color)
    {
        outlineWidth_ = width;
        outlineColor_ = color;
    }
    void SetLayerOrder(int layerOrder) { layerOrder_ = layerOrder; }
    void SetIsVisible(bool isVisible) { isVisible_ = isVisible; }

    Vector2 MeasureSize() const;

    // PropertyBinder 用
    Vector2* GetPositionPtr() { return &position_; }
    Vector2* GetScreenAnchorPtr() { return &screenAnchor_; }
    Vector2* GetPivotPtr() { return &pivot_; }
    int32_t* GetAlignPtr() { return reinterpret_cast<int32_t*>(&align_); }
    int32_t* GetDirectionPtr() { return reinterpret_cast<int32_t*>(&direction_); }
    float* GetCharacterSpacingPtr() { return &characterSpacing_; }
    float* GetWrapLengthPtr() { return &wrapLength_; }
    float* GetFontSizePtr() { return &fontSize_; }
    float* GetLineSpacingPtr() { return &lineSpacing_; }
    Vector4* GetColorPtr() { return &color_; }
    float* GetOutlineWidthPtr() { return &outlineWidth_; }
    Vector4* GetOutlineColorPtr() { return &outlineColor_; }
    int* GetLayerOrderPtr() { return &layerOrder_; }
    bool* GetIsVisiblePtr() { return &isVisible_; }

private:
    // 1 文字の配置 (文章の左上からの位置)
    struct GlyphPlacement
    {
        const Glyph* glyph = nullptr;
        Vector2 center;
        Vector2 size;
        float rotation = 0.0f;
    };

    struct TextLayout
    {
        std::vector<GlyphPlacement> glyphs;
        Vector2 blockSize;
    };

    TextLayout BuildLayout() const;
    float GetDisplayScale() const;
    // 行の方向に進む量
    float GetAdvance(char32_t codepoint, const Glyph& glyph) const;
    void SubmitGlyphs(const TextLayout& layout, const Vector2& blockTopLeft,
        const Vector4& color, float outlineThickness) const;

    Engine* engine_ = nullptr;
    const Font* font_ = nullptr;
    std::string fontName_;

    std::string text_;
    std::u32string codepoints_;

    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 screenAnchor_ = { 0.0f, 0.0f };
    Vector2 pivot_ = { 0.0f, 0.0f };
    TextAlign align_ = TextAlign::Left;
    TextDirection direction_ = TextDirection::Horizontal;

    float fontSize_ = 32.0f;
    float lineSpacing_ = 1.0f;
    float characterSpacing_ = 0.0f;
    float wrapLength_ = 0.0f;
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    float outlineWidth_ = 0.0f;
    Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
    int layerOrder_ = 0;
    bool isVisible_ = true;
};

}
