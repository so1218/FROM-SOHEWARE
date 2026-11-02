#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

// 画面左上に出すポーズメニュー
class PauseMenuHint : public FS::GameObject
{
public:
    PauseMenuHint(FS::Engine* engine);

    void Initialize() override;
    void Draw() override;
    void DebugDraw() override;

private:
    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Sprite> background_;
    std::unique_ptr<FS::Sprite> keyIcon_;
    std::unique_ptr<FS::Sprite> buttonIcon_;
    std::unique_ptr<FS::Text> separatorText_;
    std::unique_ptr<FS::Text> labelText_;
    std::unique_ptr<FS::Text> editorText_;

    // パラメータ
    FS::Vector2 position_ = { 24.0f, 24.0f };   // 画面左上から
    float fontSize_ = 20.0f;
    float iconSize_ = 34.0f;
    float spacing_ = 8.0f;
    float padding_ = 10.0f;
    FS::Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 0.9f };
    FS::Vector4 editorColor_ = { 1.0f, 0.78f, 0.3f, 1.0f };
    FS::Vector4 backgroundColor_ = { 0.0f, 0.0f, 0.0f, 0.45f };
};
