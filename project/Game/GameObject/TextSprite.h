#pragma once
#include "Engine.h"
#include "Sprite.h"

#include <string>
#include <vector>
#include <memory>

// フォントアトラスの定義
struct FontData
{
    uint32_t textureHandle; // フォントテクスチャのハンドル
    float charWidth;        // フォントテクスチャ内での1文字の幅 (UVスケール 0.0～1.0)
    float charHeight;       // フォントテクスチャ内での1文字の高さ (UVスケール 0.0～1.0)
    float charPixelWidth;   // 画面上での1文字の描画幅（ピクセルなど、任意単位）
};

class TextSprite
{
public:
    TextSprite(Engine* engine, const FontData& fontData);

    void SetText(const std::string& text);
    void Draw();

    // 諸々変更できるようにするセッター
    void SetPosition(const Vector2& position) { position_ = position; UpdateSprites(); }
    void SetRotation(float rotation) { rotation_ = rotation; UpdateSprites(); }
    void SetSize(float overallScale) { overallScale_ = overallScale; UpdateSprites(); }

private:
    Engine* engine_ = nullptr;
    FontData fontData_;

    std::vector<std::unique_ptr<Sprite>> sprites_; // 文字ごとのスプライト
    std::string currentText_;

    Vector2 position_ = { 0.0f, 0.0f };
    float overallScale_ = 1.0f; // 全体の拡大率
    float rotation_ = 0.0f;

    // 文字列を解析し、各スプライトのUVと位置を更新する
    void UpdateSprites();
};