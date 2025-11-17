#include "TextSprite.h"

TextSprite::TextSprite(Engine* engine, const FontData& fontData)
    : engine_(engine), fontData_(fontData)
{
    // フォントテクスチャを設定
    for (const auto& sprite : sprites_)
    {
        sprite->SetTextureHandle(fontData_.textureHandle);
    }
}

void TextSprite::SetText(const std::string& text)
{
    if (currentText_ == text) return;
    currentText_ = text;

    // 現在のスプライト数をリサイズ
    sprites_.clear();
    for (size_t i = 0; i < text.length(); ++i)
    {
        sprites_.push_back(std::make_unique<Sprite>(engine_));
        sprites_.back()->SetTextureHandle(fontData_.textureHandle);
        // 各文字の描画サイズを設定
        sprites_.back()->SetSize({ fontData_.charPixelWidth, fontData_.charPixelWidth });
        sprites_.back()->SetColor(0xFFFFFFFF); // デフォルト色
    }

    UpdateSprites();
}

void TextSprite::UpdateSprites()
{
    // 各文字の原点からの相対位置
    float currentX = 0.0f;
    float charDrawWidth = fontData_.charPixelWidth * overallScale_;

    // 文字列の中心を原点とするためのオフセット
    float totalWidth = currentText_.length() * charDrawWidth;
    float startX = -totalWidth / 2.0f;

    for (size_t i = 0; i < currentText_.length(); ++i)
    {
        Sprite* sprite = sprites_[i].get();
        char c = currentText_[i];

        // 1. 文字の描画位置 (中心を考慮し、全体の回転とスケールを適用)
        Vector2 charRelativePos = { startX + currentX + charDrawWidth / 2.0f, 0.0f };

        // 回転の適用
        float cosR = cos(rotation_);
        float sinR = sin(rotation_);
        Vector2 rotatedPos = {
            charRelativePos.x * cosR - charRelativePos.y * sinR,
            charRelativePos.x * sinR + charRelativePos.y * cosR
        };

        // 最終的な位置
        sprite->SetPosition({ position_.x + rotatedPos.x, position_.y + rotatedPos.y });
        sprite->SetRotation(rotation_); // 各文字も全体と同じ角度に回転

        // 2. 文字のUV設定（テクスチャ内のどの部分を描画するか）
        // ' ' は無視し、'!'から順に文字が並んでいると仮定
        // 文字の順番は、フォントアトラスの定義によって調整が必要です。
        int charIndex = c - '!'; // 例: ASCIIの'!'をインデックス0とする

        // フォントアトラス内の列数
        const int COLUMNS = 16;

        int x = charIndex % COLUMNS;
        int y = charIndex / COLUMNS;

        WorldTransform uv;
        uv.scale_ = { fontData_.charWidth, fontData_.charHeight, 1.0f };
        uv.translation_ = { x * fontData_.charWidth, y * fontData_.charHeight, 0.0f };
        sprite->SetUVTransform(uv);

        // 描画サイズをスケールに合わせて調整
        sprite->SetSize({ charDrawWidth, charDrawWidth });

        currentX += charDrawWidth / overallScale_; // 次の文字へ
    }
}

void TextSprite::Draw()
{
    for (const auto& sprite : sprites_)
    {
        sprite->Draw();
    }
}