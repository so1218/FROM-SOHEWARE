#include "pch.h"
#include "GameUI.h"
#include "TimeManager.h"

using namespace FS;

GameUI::GameUI(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "GameUI");

    // フォントを省略すると FontList.csv の先頭のフォントを使う
    instructionText_ = std::make_unique<Text>(engine_);

    // フォントを名前で指定する
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

    // 文字列はコードで作るので、配置と見た目だけ登録する
    binder_->BindTextElement("TimeText", timeText_.get(), false);
    binder_->BindString("TimeFormat", "時間 {:.1f} 秒",
        [this](const std::string& newFormat) { timeFormat_ = newFormat; });
}

void GameUI::Update()
{
    const float totalTime = TimeManager::GetInstance()->GetTotalTime();

    // 書式を ImGui で編集している途中は {} が崩れることがあるので、そのときは書式をそのまま出す
    try
    {
        timeText_->SetText(std::vformat(timeFormat_, std::make_format_args(totalTime)));
    }
    catch (const std::format_error&)
    {
        timeText_->SetText(timeFormat_);
    }
}

void GameUI::Draw()
{
    if (!isVisible_) return;

    instructionText_->Draw();
    timeText_->Draw();
    messageText_->Draw();
    stageNameText_->Draw();
}

void GameUI::DebugDraw()
{
#if ENABLE_IMGUI
    ImGui::Begin("ゲームUI調整");
    binder_->Draw("IsVisible", "表示切り替え");

    ImGui::SeparatorText("テキスト");
    binder_->DrawTextElement("InstructionText", "操作方法");
    binder_->DrawTextElement("MessageText", "メッセージ");
    binder_->DrawTextElement("StageNameText", "ステージ名");
    binder_->DrawTextElement("TimeText", "経過時間");
    binder_->Draw("TimeFormat", "経過時間の書式");
    ImGui::End();
#endif
}