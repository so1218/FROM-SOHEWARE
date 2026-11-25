#include "pch.h"
#include "GameUI.h"
#include "ImGuiManager.h"
#include "EnvironmentManager.h"
#include "TimeManager.h"

using namespace FS;

GameUI::GameUI(Engine* engine) : engine_(engine)
{
    binder_ = std::make_unique<FS::PropertyBinder>(engine_, "GameUI");

    timeText_ = std::make_unique<Text>(engine_, "Main");
    timeText_->SetAnchor(UIAnchor::TopRight);
    timeText_->SetPosition({ -20.0f, 20.0f });
    timeText_->SetFontSize(28.0f);

    messageText_ = std::make_unique<Text>(engine_);

    // 左上に、目的とその下に制限時間の線を出す
    objectiveTitleText_ = std::make_unique<Text>(engine_, "Shippori");
    objectiveSubText_ = std::make_unique<Text>(engine_, "Shippori");
    objectiveSubText_->SetText("敵を倒して金を手に入れろ");
    for (Text* text : { objectiveTitleText_.get(), objectiveSubText_.get() })
    {
        text->SetScreenAnchor({ 0.03f, 0.17f });
        text->SetPivot({ 0.0f, 0.5f });
        text->SetAlign(TextAlign::Left);
        text->SetShadow({ 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f, 0.85f }, 4.0f, 0.6f);
    }
    objectiveTitleText_->SetFontSize(40.0f);
    objectiveTitleText_->SetColor({ 0.95f, 0.9f, 0.8f, 1.0f });
    objectiveSubText_->SetPosition({ 0.0f, 36.0f });
    objectiveSubText_->SetFontSize(22.0f);
    objectiveSubText_->SetColor({ 1.0f, 1.0f, 1.0f, 0.85f });

    timeBarBack_ = std::make_unique<Sprite>(engine_);
    timeBarStorm_ = std::make_unique<Sprite>(engine_);
    timeBarFill_ = std::make_unique<Sprite>(engine_);
    for (Sprite* sprite : { timeBarBack_.get(), timeBarStorm_.get(), timeBarFill_.get() })
    {
        sprite->SetAnchorPoint({ 0.0f, 0.5f });
    }
    SetObjective(17, 19, 20);
}

void GameUI::SetObjective(int startHour, int stormHour, int endHour)
{
    startHour_ = static_cast<float>(startHour);
    stormHour_ = static_cast<float>(stormHour);
    endHour_ = static_cast<float>(endHour);
    objectiveTitleText_->SetText(std::to_string(endHour) + "時まで生き残れ");
}

void GameUI::Initialize()
{
    binder_->Bind("IsVisible", &isVisible_, true);

    binder_->BindTextElement("MessageText", messageText_.get());
    // 中身は遊ぶ時刻から決めるので、保存しない
    binder_->BindTextElement("ObjectiveTitleText", objectiveTitleText_.get(), false);
    binder_->BindTextElement("ObjectiveSubText", objectiveSubText_.get(), false);
    binder_->Bind("ObjectiveFadeTime", &objectiveFadeTime_, 0.6f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("TimeBarScreenAnchor", &timeBarScreenAnchor_, { 0.03f, 0.17f }, 0.01f, 0.0f, 1.0f);
    binder_->Bind("TimeBarPos", &timeBarPos_, { 0.0f, 62.0f }, 1.0f);
    binder_->Bind("TimeBarSize", &timeBarSize_, { 320.0f, 6.0f }, 1.0f, 0.0f, 1280.0f);
    binder_->BindColor("TimeBarBackColor", &timeBarBackColor_, { 1.0f, 1.0f, 1.0f, 0.2f });
    binder_->BindColor("TimeBarStormColor", &timeBarStormColor_, { 0.6f, 0.1f, 0.08f, 0.6f });
    binder_->BindColor("TimeBarFillColor", &timeBarFillColor_, { 1.0f, 0.95f, 0.85f, 0.9f });

    // 配置と見た目だけ登録
    binder_->BindTextElement("TimeText", timeText_.get(), false);
    binder_->BindString("TimeFormat", "{}:{:02}",
        [this](const std::string& newFormat) { timeFormat_ = newFormat; });
    binder_->Bind("TimeMinuteStep", &minuteStep_, 1, 1.0f, 1, 60);
    binder_->Bind("TimeFlashTime", &clockFlashTime_, 1.2f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("TimeFlashScale", &clockFlashScale_, 1.5f, 0.01f, 1.0f, 5.0f);
    binder_->BindColor("TimeFlashColor", &clockFlashColor_, { 1.0f, 0.35f, 0.25f, 1.0f });
}

void GameUI::Update()
{
    const float unscaledDeltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    clockFlashTimer_ += unscaledDeltaTime;

    const float fadeStep = unscaledDeltaTime / std::max(objectiveFadeTime_, 0.001f);
    objectiveVisibility_ = isObjectiveVisible_
        ? std::min(objectiveVisibility_ + fadeStep, 1.0f)
        : std::max(objectiveVisibility_ - fadeStep, 0.0f);

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

    // 光らせている間だけ大きさと色を変え、描いたら保存している値に戻す
    const float flashRatio = 1.0f - std::clamp(clockFlashTimer_ / std::max(clockFlashTime_, 0.001f), 0.0f, 1.0f);
    if (flashRatio > 0.0f)
    {
        const float flash = flashRatio * flashRatio;
        const float baseFontSize = *timeText_->GetFontSizePtr();
        const Vector4 baseColor = *timeText_->GetColorPtr();
        *timeText_->GetFontSizePtr() = baseFontSize * (1.0f + (clockFlashScale_ - 1.0f) * flash);
        *timeText_->GetColorPtr() = baseColor + (clockFlashColor_ - baseColor) * flash;
        timeText_->Draw();
        *timeText_->GetFontSizePtr() = baseFontSize;
        *timeText_->GetColorPtr() = baseColor;
    }
    else
    {
        timeText_->Draw();
    }

    DrawObjective();
}

void GameUI::DrawObjective()
{
    if (objectiveVisibility_ <= 0.0f)
    {
        return;
    }

    for (Text* text : { objectiveTitleText_.get(), objectiveSubText_.get() })
    {
        text->SetOpacity(objectiveVisibility_);
        text->Draw();
    }

    // 始まりから終わりまでの線
    const float totalHours = std::max(endHour_ - startHour_, 0.001f);
    const float timeOfDay = EnvironmentManager::GetInstance()->GetTimeOfDay();
    const float elapsedRatio = std::clamp((timeOfDay - startHour_) / totalHours, 0.0f, 1.0f);
    const float stormStartRatio = std::clamp((stormHour_ - startHour_) / totalHours, 0.0f, 1.0f);

    auto drawBar = [&](Sprite& sprite, float startRatio, float endRatio, const Vector4& color)
        {
            if (endRatio <= startRatio)
            {
                return;
            }
            Vector4 drawColor = color;
            drawColor.w *= objectiveVisibility_;
            sprite.SetScreenAnchor(timeBarScreenAnchor_);
            sprite.SetPosition({ timeBarPos_.x + timeBarSize_.x * startRatio, timeBarPos_.y });
            sprite.SetSize({ timeBarSize_.x * (endRatio - startRatio), timeBarSize_.y });
            sprite.SetColor(drawColor);
            sprite.Draw();
        };
    drawBar(*timeBarBack_, 0.0f, stormStartRatio, timeBarBackColor_);
    drawBar(*timeBarStorm_, stormStartRatio, 1.0f, timeBarStormColor_);
    drawBar(*timeBarFill_, 0.0f, elapsedRatio, timeBarFillColor_);
}

void GameUI::DebugGui()
{
    ImGui::Begin("ゲームUI調整");
    binder_->Draw("IsVisible", "表示切り替え");

    ImGui::SeparatorText("テキスト");
    binder_->DrawTextElement("MessageText", "メッセージ");
    binder_->DrawTextElement("ObjectiveTitleText", "目的の見出し");
    binder_->DrawTextElement("ObjectiveSubText", "目的");
    binder_->DrawTextElement("TimeText", "時刻");
    binder_->Draw("TimeFormat", "時刻の書式 ({} に時、分の順で入る)");
    binder_->Draw("TimeMinuteStep", "分の刻み (分)");
    binder_->Draw("TimeFlashTime", "本番開始で時計を光らせる時間 (秒)");
    binder_->Draw("TimeFlashScale", "光らせるときの大きさ (倍)");
    binder_->Draw("TimeFlashColor", "光らせるときの色");
    if (ImGui::Button("時計を光らせてみる"))
    {
        FlashClock();
    }

    ImGui::SeparatorText("目的と制限時間");
    binder_->Draw("ObjectiveFadeTime", "出るまで (秒)");
    binder_->Draw("TimeBarScreenAnchor", "制限時間の線の基準 (画面の割合)");
    binder_->Draw("TimeBarPos", "制限時間の線の位置 (基準から)");
    binder_->Draw("TimeBarSize", "制限時間の線の長さと太さ");
    binder_->Draw("TimeBarBackColor", "線の下地の色");
    binder_->Draw("TimeBarStormColor", "雷雨の区間の色");
    binder_->Draw("TimeBarFillColor", "過ぎた時間の色");
    ImGui::End();
}