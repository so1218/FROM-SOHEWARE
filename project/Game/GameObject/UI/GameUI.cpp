#include "pch.h"
#include "GameUI.h"
#include "ImGuiManager.h"

using namespace FS;

GameUI::GameUI(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "GameUI");

    instructionText_ = std::make_unique<Text>(engine_);

    timeText_ = std::make_unique<Text>(engine_, "Main");
    timeText_->SetAnchor(UIAnchor::TopRight);
    timeText_->SetPosition({ -20.0f, 20.0f });
    timeText_->SetFontSize(28.0f);

    messageText_ = std::make_unique<Text>(engine_);
    stageNameText_ = std::make_unique<Text>(engine_);
}

void GameUI::Initialize()
{
    binder_->Bind("IsVisible", &isVisible_, true);

    binder_->BindTextElement("InstructionText", instructionText_.get());
    binder_->BindTextElement("MessageText", messageText_.get());
    binder_->BindTextElement("StageNameText", stageNameText_.get());

    // 配置と見た目だけ登録
    binder_->BindTextElement("TimeText", timeText_.get(), false);
    binder_->BindString("TimeFormat", "{}:{:02}",
        [this](const std::string& newFormat) { timeFormat_ = newFormat; });
}

void GameUI::Update()
{
    const int totalSeconds = static_cast<int>(std::ceil(std::max(remainingTime_, 0.0f)));
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;

    // 書式を ImGui で編集している途中は 崩れることがあるので、そのときは書式をそのまま出す
    try
    {
        timeText_->SetText(std::vformat(timeFormat_, std::make_format_args(minutes, seconds)));
    }
    catch (const std::format_error&)
    {
        timeText_->SetText(timeFormat_);
    }
}

void GameUI::Draw()
{
    if (!isVisible_) return;

    // カメラの操作説明なので、エディター表示のときだけ出す
    if (ImGuiManager::IsGuiVisible())
    {
        messageText_->Draw();
    }

    if (!isGameplayVisible_) return;

    instructionText_->Draw();
    timeText_->Draw();
    stageNameText_->Draw();
}

void GameUI::DebugGui()
{
    ImGui::Begin("ゲームUI調整");
    binder_->Draw("IsVisible", "表示切り替え");

    ImGui::SeparatorText("テキスト");
    binder_->DrawTextElement("InstructionText", "操作方法");
    binder_->DrawTextElement("MessageText", "メッセージ");
    binder_->DrawTextElement("StageNameText", "ステージ名");
    binder_->DrawTextElement("TimeText", "残り時間");
    binder_->Draw("TimeFormat", "残り時間の書式 ({} に分、秒の順で入る)");
    ImGui::End();
}