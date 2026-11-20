#include "pch.h"
#include "GameUI.h"
#include "ImGuiManager.h"
#include "EnvironmentManager.h"

using namespace FS;

GameUI::GameUI(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "GameUI");

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

    binder_->BindTextElement("MessageText", messageText_.get());
    binder_->BindTextElement("StageNameText", stageNameText_.get());

    // 配置と見た目だけ登録
    binder_->BindTextElement("TimeText", timeText_.get(), false);
    binder_->BindString("TimeFormat", "{}:{:02}",
        [this](const std::string& newFormat) { timeFormat_ = newFormat; });
    binder_->Bind("TimeMinuteStep", &minuteStep_, 10, 1.0f, 1, 60);
}

void GameUI::Update()
{
    // ゲームの中の時刻 (0 ~ 24 時間)
    const float timeOfDay = EnvironmentManager::GetInstance()->GetTimeOfDay();
    const int totalMinutes = static_cast<int>(timeOfDay * 60.0f) % (24 * 60);
    const int hours = totalMinutes / 60;
    const int step = std::max(minuteStep_, 1);
    const int minutes = (totalMinutes % 60) / step * step;

    // 書式を ImGui で編集している途中は 崩れることがあるので、そのときは書式をそのまま出す
    try
    {
        timeText_->SetText(std::vformat(timeFormat_, std::make_format_args(hours, minutes)));
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

    timeText_->Draw();
    stageNameText_->Draw();
}

void GameUI::DebugGui()
{
    ImGui::Begin("ゲームUI調整");
    binder_->Draw("IsVisible", "表示切り替え");

    ImGui::SeparatorText("テキスト");
    binder_->DrawTextElement("MessageText", "メッセージ");
    binder_->DrawTextElement("StageNameText", "ステージ名");
    binder_->DrawTextElement("TimeText", "時刻");
    binder_->Draw("TimeFormat", "時刻の書式 ({} に時、分の順で入る)");
    binder_->Draw("TimeMinuteStep", "分の刻み (分)");
    ImGui::End();
}