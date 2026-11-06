#include "pch.h"
#include "PauseMenu.h"
#include "ImGuiManager.h"
#include "TimeManager.h"
#include "Input.h"

using namespace FS;

PauseMenu::PauseMenu(Engine* engine)
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "PauseMenu");

    overlay_ = std::make_unique<Sprite>(engine_);
    overlay_->SetScreenAnchor({ 0.0f, 0.0f });
    overlay_->SetAnchorPoint({ 0.0f, 0.0f });
    overlay_->SetLayerOrder(50);

    cursorBand_ = std::make_unique<Sprite>(engine_);
    cursorBand_->SetScreenAnchor({ 0.5f, 0.5f });
    cursorBand_->SetAnchorPoint({ 0.5f, 0.5f });
    cursorBand_->SetLayerOrder(51);

    headerText_ = std::make_unique<Text>(engine_);
    headerText_->SetText("PAUSE");
    headerText_->SetCharacterSpacing(10.0f);
    itemText_ = std::make_unique<Text>(engine_);
    descriptionText_ = std::make_unique<Text>(engine_);
    questionText_ = std::make_unique<Text>(engine_);
    questionText_->SetText("本当に終了しますか？");

    for (Text* text : { headerText_.get(), itemText_.get(), descriptionText_.get(), questionText_.get() })
    {
        text->SetAnchor(UIAnchor::Center);
        text->SetAlign(TextAlign::Center);
        text->SetLayerOrder(52);
    }
}

void PauseMenu::Initialize()
{
    binder_->Bind("FadeTime", &fadeTime_, 0.15f, 0.01f, 0.01f, 2.0f);
    binder_->BindColor("OverlayColor", &overlayColor_, { 0.0f, 0.0f, 0.0f, 0.65f });
    binder_->Bind("HeaderFontSize", &headerFontSize_, 40.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("HeaderOffsetY", &headerOffsetY_, -120.0f, 1.0f);
    binder_->Bind("ItemFontSize", &itemFontSize_, 30.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ItemSpacing", &itemSpacing_, 56.0f, 0.5f, 0.0f, 300.0f);
    binder_->Bind("CursorBandSize", &cursorBandSize_, { 460.0f, 48.0f }, 1.0f, 1.0f, 2000.0f);
    binder_->BindColor("CursorBandColor", &cursorBandColor_, { 1.0f, 1.0f, 1.0f, 0.12f });
    binder_->BindColor("SelectedColor", &selectedColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->BindColor("UnselectedColor", &unselectedColor_, { 1.0f, 1.0f, 1.0f, 0.45f });
    binder_->BindColor("EditorColor", &editorColor_, { 1.0f, 0.78f, 0.3f, 1.0f });
    binder_->Bind("DescriptionFontSize", &descriptionFontSize_, 20.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("DescriptionOffsetY", &descriptionOffsetY_, 40.0f, 1.0f);

    isOpen_ = false;
    openAmount_ = 0.0f;
    selectedItem_.reset();
}

void PauseMenu::Update()
{
    const float deltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    openAmount_ = std::clamp(openAmount_ + (isOpen_ ? deltaTime : -deltaTime) / fadeTime_, 0.0f, 1.0f);

    if (IsToggleTriggered())
    {
        if (isOpen_)
        {
            Close();
        }
        else if (canOpen_)
        {
            Open();
        }
        return;
    }

    if (!isOpen_)
    {
        return;
    }

    if (isConfirmingQuit_)
    {
        UpdateQuitConfirm();
        return;
    }

    if (IsCancelTriggered())
    {
        Close();
        return;
    }

    UpdateCursor();

    if (IsConfirmTriggered())
    {
        if (items_[cursorIndex_] == Item::QuitGame)
        {
            // 間違えて押しても終わらないよう、先に確認する。最初は「やめる」を選んでおく
            isConfirmingQuit_ = true;
            quitConfirmIndex_ = 0;
            return;
        }
        selectedItem_ = items_[cursorIndex_];
        Close();
    }
}

void PauseMenu::UpdateQuitConfirm()
{
    if (IsCancelTriggered())
    {
        isConfirmingQuit_ = false;
        return;
    }

    if (IsUpTriggered() || IsDownTriggered())
    {
        quitConfirmIndex_ = 1 - quitConfirmIndex_;
    }

    if (IsConfirmTriggered())
    {
        if (quitConfirmIndex_ == 1)
        {
            selectedItem_ = Item::QuitGame;
            Close();
        }
        else
        {
            isConfirmingQuit_ = false;
        }
    }
}

void PauseMenu::Open()
{
    items_ = { Item::Resume, Item::ToggleEditor };
    if (canReturnToTitle_)
    {
        items_.push_back(Item::ReturnToTitle);
    }
    items_.push_back(Item::QuitGame);
    cursorIndex_ = 0;
    isConfirmingQuit_ = false;
    isOpen_ = true;
}

void PauseMenu::Close()
{
    isOpen_ = false;
}

void PauseMenu::UpdateCursor()
{
    const int itemCount = static_cast<int>(items_.size());
    if (IsUpTriggered())
    {
        cursorIndex_ = (cursorIndex_ + itemCount - 1) % itemCount;
    }
    if (IsDownTriggered())
    {
        cursorIndex_ = (cursorIndex_ + 1) % itemCount;
    }
}

std::optional<PauseMenu::Item> PauseMenu::TakeSelectedItem()
{
    std::optional<Item> item = selectedItem_;
    selectedItem_.reset();
    return item;
}

std::string PauseMenu::GetLabel(Item item) const
{
    switch (item)
    {
    case Item::Resume:        return "ゲームに戻る";
    case Item::ToggleEditor:  return ImGuiManager::IsGuiVisible() ? "エディターを閉じる" : "エディターを開く";
    case Item::ReturnToTitle: return "タイトルに戻る";
    case Item::QuitGame:      return "ゲームを終了";
    }
    return "";
}

bool PauseMenu::IsToggleTriggered() const
{
    auto& input = Input::GetInstance();
    return input.IsKeyTriggered(DIK_ESCAPE) || input.IsControllerButtonTriggered(0, Input::ButtonMenu);
}

bool PauseMenu::IsConfirmTriggered() const
{
    auto& input = Input::GetInstance();
    return input.IsKeyTriggered(DIK_SPACE) || input.IsKeyTriggered(DIK_RETURN) || input.IsControllerButtonTriggered(0, Input::ButtonA);
}

bool PauseMenu::IsCancelTriggered() const
{
    return Input::GetInstance().IsControllerButtonTriggered(0, Input::ButtonB);
}

bool PauseMenu::IsUpTriggered() const
{
    auto& input = Input::GetInstance();
    return input.IsKeyTriggered(DIK_W) || input.IsKeyTriggered(DIK_UP)
        || input.IsControllerButtonTriggered(0, Input::ButtonUp) || input.IsStickUpTriggered(0, Input::LeftStick);
}

bool PauseMenu::IsDownTriggered() const
{
    auto& input = Input::GetInstance();
    return input.IsKeyTriggered(DIK_S) || input.IsKeyTriggered(DIK_DOWN)
        || input.IsControllerButtonTriggered(0, Input::ButtonDown) || input.IsStickDownTriggered(0, Input::LeftStick);
}

void PauseMenu::Draw()
{
    if (openAmount_ <= 0.0f)
    {
        return;
    }

    Vector4 overlayColor = overlayColor_;
    overlayColor.w *= openAmount_;
    overlay_->SetSize({ Engine::GetUIWidth(), Engine::GetUIHeight() });
    overlay_->SetColor(overlayColor);
    overlay_->Draw();

    if (isConfirmingQuit_)
    {
        questionText_->SetFontSize(itemFontSize_);
        questionText_->SetPosition({ 0.0f, headerOffsetY_ });
        questionText_->SetColor(selectedColor_);
        questionText_->SetOpacity(openAmount_);
        questionText_->Draw();

        DrawItems({ "やめる", "終了する" }, quitConfirmIndex_, -1);
        return;
    }

    headerText_->SetFontSize(headerFontSize_);
    headerText_->SetPosition({ 0.0f, headerOffsetY_ });
    headerText_->SetColor(selectedColor_);
    headerText_->SetOpacity(openAmount_);
    headerText_->Draw();

    std::vector<std::string> labels;
    int editorIndex = -1;
    for (int itemIndex = 0; itemIndex < static_cast<int>(items_.size()); ++itemIndex)
    {
        labels.push_back(GetLabel(items_[itemIndex]));
        if (items_[itemIndex] == Item::ToggleEditor)
        {
            editorIndex = itemIndex;
        }
    }
    DrawItems(labels, cursorIndex_, editorIndex);

    if (cursorIndex_ == editorIndex)
    {
        const float lastItemY = itemSpacing_ * static_cast<float>(items_.size() - 1) * 0.5f;
        descriptionText_->SetText(ImGuiManager::IsGuiVisible()
            ? "ゲーム画面だけの表示に戻します"
            : "自作エンジンのエディターを開きます");
        descriptionText_->SetFontSize(descriptionFontSize_);
        descriptionText_->SetPosition({ 0.0f, lastItemY + descriptionOffsetY_ });
        descriptionText_->SetColor(editorColor_);
        descriptionText_->SetOpacity(openAmount_);
        descriptionText_->Draw();
    }
}

void PauseMenu::DrawItems(const std::vector<std::string>& labels, int cursorIndex, int highlightIndex)
{
    // 項目は画面中央を中心に縦に並べる
    const float firstItemY = -itemSpacing_ * static_cast<float>(labels.size() - 1) * 0.5f;
    for (int itemIndex = 0; itemIndex < static_cast<int>(labels.size()); ++itemIndex)
    {
        const float itemY = firstItemY + itemSpacing_ * static_cast<float>(itemIndex);
        const bool isSelected = itemIndex == cursorIndex;

        if (isSelected)
        {
            Vector4 bandColor = cursorBandColor_;
            bandColor.w *= openAmount_;
            cursorBand_->SetPosition({ 0.0f, itemY });
            cursorBand_->SetSize(cursorBandSize_);
            cursorBand_->SetColor(bandColor);
            cursorBand_->Draw();
        }

        Vector4 itemColor = isSelected ? selectedColor_ : unselectedColor_;
        if (itemIndex == highlightIndex)
        {
            itemColor = { editorColor_.x, editorColor_.y, editorColor_.z, isSelected ? editorColor_.w : editorColor_.w * 0.7f };
        }

        itemText_->SetText(labels[itemIndex]);
        itemText_->SetFontSize(itemFontSize_);
        itemText_->SetPosition({ 0.0f, itemY });
        itemText_->SetColor(itemColor);
        itemText_->SetOpacity(openAmount_);
        itemText_->Draw();
    }
}

void PauseMenu::DebugGui()
{
    ImGui::Begin("ポーズメニュー");
    binder_->Draw("FadeTime", "開くまでの時間 (秒)");
    binder_->Draw("OverlayColor", "背景を暗くする色");
    binder_->Draw("HeaderFontSize", "PAUSE の大きさ");
    binder_->Draw("HeaderOffsetY", "PAUSE の位置 (画面中央から)");
    binder_->Draw("ItemFontSize", "項目の文字の大きさ");
    binder_->Draw("ItemSpacing", "項目の間隔");
    binder_->Draw("CursorBandSize", "選択中の帯の大きさ");
    binder_->Draw("CursorBandColor", "選択中の帯の色");
    binder_->Draw("SelectedColor", "選択中の文字の色");
    binder_->Draw("UnselectedColor", "ほかの項目の文字の色");
    binder_->Draw("EditorColor", "エディターの項目の色");
    binder_->Draw("DescriptionFontSize", "説明の文字の大きさ");
    binder_->Draw("DescriptionOffsetY", "説明の位置 (最後の項目の下から)");
    ImGui::End();
}
