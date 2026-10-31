#pragma once
#include "Vector2.h"
#include "Vector4.h"

namespace FS
{

class Engine;

// 1 文字分の情報
struct Glyph
{
    Vector4 uvRect = { 0.0f, 0.0f, 0.0f, 0.0f }; // アトラス上の位置 (u0, v0, u1, v1)
    Vector2 size = { 0.0f, 0.0f };               // 文字の画像の大きさ
    Vector2 offset = { 0.0f, 0.0f };             // 行の左上からのずれ
    float advance = 0.0f;                        // 次の文字までの距離
};

// フォントファイルから SDF の文字アトラスを作る
class Font
{
public:
    bool Load(Engine* engine, const std::string& name, const std::string& fontPath,
        float pixelHeight, const std::u32string& characters);

    const Glyph* FindGlyph(char32_t codepoint) const;

    uint32_t GetTextureIndex() const { return textureIndex_; }
    float GetPixelHeight() const { return pixelHeight_; }
    float GetLineHeight() const { return lineHeight_; }
    float GetAscent() const { return ascent_; }
    // 全角 1 文字分の大きさと、その上端からベースラインまでの距離 (縦書き用)
    float GetEmSize() const { return emSize_; }
    float GetEmAscent() const { return emAscent_; }

    // 文字の外側に持つ距離の幅。縁取りの太さの上限
    static constexpr int kSdfPadding = 10;
    // アトラス 1 ピクセルあたりの距離の変化
    static constexpr float kSdfDistancePerPixel = (128.0f / kSdfPadding) / 255.0f;

private:
    static constexpr int kAtlasSize = 2048;
    static constexpr int kGlyphSpacing = 2;        // アトラス上の文字同士のすき間
    static constexpr uint32_t kCacheVersion = 1;   // キャッシュの形を変えたら上げる

    // フォントファイルから距離のアトラスを作る
    bool BuildAtlas(const std::string& fontPath, const std::u32string& characters,
        std::vector<unsigned char>& distanceField);

    // 作ったアトラスをファイルに保存し、次回からはそれを読む
    bool LoadCache(const std::string& cachePath, uint64_t cacheKey, std::vector<unsigned char>& distanceField);
    void SaveCache(const std::string& cachePath, uint64_t cacheKey, const std::vector<unsigned char>& distanceField) const;

    std::unordered_map<char32_t, Glyph> glyphs_;
    uint32_t textureIndex_ = 0;
    float pixelHeight_ = 0.0f;
    float lineHeight_ = 0.0f;
    float ascent_ = 0.0f;
    float emSize_ = 0.0f;
    float emAscent_ = 0.0f;
};

}
