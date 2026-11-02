#pragma once
#include "GameObject.h"
#include "PropertyBinder.h"
#include "Sprite.h"
#include "Text.h"

// ポーズメニュー
class PauseMenu : public FS::GameObject
{
public:
    enum class Item
    {
        Resume,
        ToggleEditor,
        ReturnToTitle,
    };

    PauseMenu(FS::Engine* engine);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugDraw() override;

    void SetCanOpen(bool canOpen) { canOpen_ = canOpen; }
    void SetCanReturnToTitle(bool canReturnToTitle) { canReturnToTitle_ = canReturnToTitle; }

    bool IsOpen() const { return isOpen_; }
    // 決定された項目を受け取る。受け取ると空になる
    std::optional<Item> TakeSelectedItem();

private:
    void Open();
    void Close();
    void UpdateCursor();
    std::string GetLabel(Item item) const;

    bool IsToggleTriggered() const;
    bool IsConfirmTriggered() const;
    bool IsCancelTriggered() const;
    bool IsUpTriggered() const;
    bool IsDownTriggered() const;

    FS::Engine* engine_ = nullptr;
    std::unique_ptr<FS::PropertyBinder> binder_;
    std::unique_ptr<FS::Sprite> overlay_;
    std::unique_ptr<FS::Sprite> cursorBand_;
    std::unique_ptr<FS::Text> headerText_;
    std::unique_ptr<FS::Text> itemText_;   // 1 つを使い回して項目の数だけ描く
    std::unique_ptr<FS::Text> descriptionText_;

    bool canOpen_ = false;
    bool canReturnToTitle_ = false;
    bool isOpen_ = false;
    float openAmount_ = 0.0f;   // 0 = 閉じている, 1 = 開ききった
    std::vector<Item> items_;
    int cursorIndex_ = 0;
    std::optional<Item> selectedItem_;

    // パラメータ
    float fadeTime_ = 0.15f;
    FS::Vector4 overlayColor_ = { 0.0f, 0.0f, 0.0f, 0.65f };
    float headerFontSize_ = 40.0f;
    float headerOffsetY_ = -120.0f;   // 画面中央から
    float itemFontSize_ = 30.0f;
    float itemSpacing_ = 56.0f;
    FS::Vector2 cursorBandSize_ = { 460.0f, 48.0f };
    FS::Vector4 cursorBandColor_ = { 1.0f, 1.0f, 1.0f, 0.12f };
    FS::Vector4 selectedColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    FS::Vector4 unselectedColor_ = { 1.0f, 1.0f, 1.0f, 0.45f };
    FS::Vector4 editorColor_ = { 1.0f, 0.78f, 0.3f, 1.0f };   
    float descriptionFontSize_ = 20.0f;
    float descriptionOffsetY_ = 40.0f;   // 最後の項目の下から
};
