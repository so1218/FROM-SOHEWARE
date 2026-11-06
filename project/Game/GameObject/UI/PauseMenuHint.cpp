#include "pch.h"
#include "PauseMenuHint.h"
#include "ImGuiManager.h"

using namespace FS;

PauseMenuHint::PauseMenuHint(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "PauseMenuHint");

    background_ = std::make_unique<Sprite>(engine_);
    background_->SetScreenAnchor({ 0.0f, 0.0f });
    background_->SetAnchorPoint({ 0.0f, 0.0f });
    background_->SetLayerOrder(40);

    keyIcon_ = std::make_unique<Sprite>(engine_);
    keyIcon_->SetTexture("keyboard_escape");
    buttonIcon_ = std::make_unique<Sprite>(engine_);
    buttonIcon_->SetTexture("xbox_button_menu");
    for (Sprite* icon : { keyIcon_.get(), buttonIcon_.get() })
    {
        icon->SetScreenAnchor({ 0.0f, 0.0f });
        icon->SetAnchorPoint({ 0.0f, 0.5f });
        icon->SetLayerOrder(41);
    }

    separatorText_ = std::make_unique<Text>(engine_);
    separatorText_->SetText("/");
    labelText_ = std::make_unique<Text>(engine_);
    labelText_->SetText("メニュー");
    editorText_ = std::make_unique<Text>(engine_);
    for (Text* text : { separatorText_.get(), labelText_.get(), editorText_.get() })
    {
        text->SetScreenAnchor({ 0.0f, 0.0f });
        text->SetPivot({ 0.0f, 0.5f });
        text->SetLayerOrder(42);
    }
}

void PauseMenuHint::Initialize()
{
    binder_->Bind("Position", &position_, { 24.0f, 24.0f }, 1.0f);
    binder_->Bind("FontSize", &fontSize_, 20.0f, 0.5f, 4.0f, 100.0f);
    binder_->Bind("IconSize", &iconSize_, 34.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("Spacing", &spacing_, 8.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("Padding", &padding_, 10.0f, 0.5f, 0.0f, 100.0f);
    binder_->BindColor("TextColor", &textColor_, { 1.0f, 1.0f, 1.0f, 0.9f });
    binder_->BindColor("EditorColor", &editorColor_, { 1.0f, 0.78f, 0.3f, 1.0f });
    binder_->BindColor("BackgroundColor", &backgroundColor_, { 0.0f, 0.0f, 0.0f, 0.45f });
}

void PauseMenuHint::Draw()
{
    editorText_->SetText(ImGuiManager::IsGuiVisible() ? "エディター表示中" : "エディター使用可能");

    const float centerY = position_.y + padding_ + iconSize_ * 0.5f;
    float cursorX = position_.x + padding_;

    auto drawText = [&](Text& text, const Vector4& color)
        {
            text.SetFontSize(fontSize_);
            text.SetPosition({ cursorX, centerY });
            text.SetColor(color);
            text.Draw();
            cursorX += text.MeasureSize().x + spacing_;
        };
    auto drawIcon = [&](Sprite& icon)
        {
            icon.SetPosition({ cursorX, centerY });
            icon.SetSize({ iconSize_, iconSize_ });
            icon.Draw();
            cursorX += iconSize_ + spacing_;
        };

    drawIcon(*keyIcon_);
    drawText(*separatorText_, textColor_);
    drawIcon(*buttonIcon_);
    drawText(*labelText_, textColor_);
    cursorX += spacing_;
    drawText(*editorText_, editorColor_);

    background_->SetPosition(position_);
    background_->SetSize({ cursorX - spacing_ + padding_ - position_.x, iconSize_ + padding_ * 2.0f });
    background_->SetColor(backgroundColor_);
    background_->Draw();
}

void PauseMenuHint::DebugGui()
{
    ImGui::Begin("操作の案内");
    binder_->Draw("Position", "位置 (画面左上から)");
    binder_->Draw("FontSize", "文字の大きさ");
    binder_->Draw("IconSize", "ボタン画像の大きさ");
    binder_->Draw("Spacing", "間隔");
    binder_->Draw("Padding", "背景の余白");
    binder_->Draw("TextColor", "文字の色");
    binder_->Draw("EditorColor", "エディターの案内の色");
    binder_->Draw("BackgroundColor", "背景の色");
    ImGui::End();
}
