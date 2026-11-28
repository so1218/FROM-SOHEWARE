#include "pch.h"
#include "ObjectiveBanner.h"
#include "UITextStyle.h"
#include "GameInput.h"
#include "TimeManager.h"
#include "UISound.h"

using namespace FS;

namespace
{
float SmoothStep01(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Ratio(float time, float duration)
{
    return std::clamp(time / std::max(duration, 0.001f), 0.0f, 1.0f);
}

void ReplaceAll(std::string& text, const std::string& from, const std::string& to)
{
    for (size_t position = text.find(from); position != std::string::npos; position = text.find(from, position + to.size()))
    {
        text.replace(position, from.size(), to);
    }
}
}

ObjectiveBanner::ObjectiveBanner(Engine* engine)
    : engine_(engine), subtitle_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "ObjectiveBanner");

    blackout_ = std::make_unique<Sprite>(engine_);
    blackout_->SetScreenAnchor({ 0.0f, 0.0f });
    blackout_->SetAnchorPoint({ 0.0f, 0.0f });
    blackout_->SetLayerOrder(49);

    // 答えるボタンは、絵の右に文字を置いてひとまとまりで中央にそろえる
    replyText_ = std::make_unique<Text>(engine_, "Maru");
    UITextStyle::ApplyMaruShadow(*replyText_);
    replyText_->SetScreenAnchor({ 0.5f, 0.5f });
    replyText_->SetPivot({ 0.0f, 0.5f });
    replyText_->SetAlign(TextAlign::Left);
    replyText_->SetLayerOrder(46);
    replyIcon_ = std::make_unique<Sprite>(engine_);
    replyIcon_->SetScreenAnchor({ 0.5f, 0.5f });
    replyIcon_->SetAnchorPoint({ 0.5f, 0.5f });
    replyIcon_->SetLayerOrder(46);
}

void ObjectiveBanner::Initialize()
{
    binder_->Bind("FadeOutTime", &fadeOutTime_, 0.8f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("HoldTime", &holdTime_, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("FadeInTime", &fadeInTime_, 0.8f, 0.01f, 0.01f, 5.0f);

    binder_->BindString("Speaker", speakerName_, [this](const std::string& text) { speakerName_ = text; });
    binder_->BindString("Line1", lineSources_[0], [this](const std::string& text) { lineSources_[0] = text; });
    binder_->BindString("Line2", lineSources_[1], [this](const std::string& text) { lineSources_[1] = text; });
    binder_->BindString("Line3", lineSources_[2], [this](const std::string& text) { lineSources_[2] = text; });
    binder_->BindString("ReplyLabel", replyLabel_, [this](const std::string& text) { replyLabel_ = text; });

    binder_->Bind("ReplyDelay", &replyDelay_, 0.3f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("LeaveTime", &leaveTime_, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("SkipDelay", &skipDelay_, 0.5f, 0.01f, 0.0f, 5.0f);

    binder_->Bind("OffsetY", &offsetY_, 30.0f, 1.0f);
    binder_->Bind("ReplyGap", &replyGap_, 60.0f, 0.5f, 0.0f, 500.0f);
    binder_->Bind("ReplyFontSize", &replyFontSize_, 26.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ReplyIconSize", &replyIconSize_, 40.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ReplyIconGap", &replyIconGap_, 10.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("ReplyPulseSpeed", &replyPulseSpeed_, 3.0f, 0.1f, 0.0f, 20.0f);
    binder_->BindColor("ReplyColor", &replyColor_, { 1.0f, 1.0f, 1.0f, 1.0f });

    // 字幕の時間・見た目・音は、今までと同じキーで保存する
    subtitle_.BindProperties(*binder_, "");

    phase_ = Phase::Hidden;
}

std::string ObjectiveBanner::ApplyPlaceholders(const std::string& text) const
{
    // ゲームの中の時刻と、現実の長さ
    const int totalSeconds = static_cast<int>(std::round(playSeconds_));
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    std::string limitText;
    if (minutes > 0)
    {
        limitText += std::to_string(minutes) + "分";
    }
    if (seconds > 0 || minutes == 0)
    {
        limitText += std::to_string(seconds) + "秒";
    }

    std::string result = text;
    ReplaceAll(result, "{hour}", std::to_string(endHour_));
    ReplaceAll(result, "{limit}", limitText);
    return result;
}

void ObjectiveBanner::Begin(bool withFade, [[maybe_unused]] bool isFirstTime, int endHour, float playSeconds)
{
    endHour_ = endHour;
    playSeconds_ = playSeconds;
    // 答えるボタンの文字は、字幕と同じフォントにする
    replyText_->SetFont(subtitle_.GetFontName());
    replyText_->SetText(replyLabel_);
    isResetRequested_ = false;
    isFinished_ = false;
    subtitle_.Stop();
    if (withFade)
    {
        ChangePhase(Phase::FadeOut);
    }
    else
    {
        ChangePhase(Phase::Show);
        StartSubtitle(0.0f);
    }
}

void ObjectiveBanner::StartSubtitle(float startDelay)
{
    std::vector<std::string> lines;
    for (const std::string& source : lineSources_)
    {
        lines.push_back(ApplyPlaceholders(source));
    }
    subtitle_.Start(speakerName_, lines, startDelay);
}

void ObjectiveBanner::Cancel()
{
    isResetRequested_ = false;
    isFinished_ = false;
    subtitle_.Stop();
    ChangePhase(Phase::Hidden);
}

bool ObjectiveBanner::TakeResetRequest()
{
    const bool isRequested = isResetRequested_;
    isResetRequested_ = false;
    return isRequested;
}

bool ObjectiveBanner::TakeFinished()
{
    const bool isFinished = isFinished_;
    isFinished_ = false;
    return isFinished;
}

void ObjectiveBanner::ChangePhase(Phase phase)
{
    phase_ = phase;
    phaseTimer_ = 0.0f;
}

void ObjectiveBanner::Update()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const float deltaTime = TimeManager::GetInstance()->GetUnscaledDeltaTime();
    phaseTimer_ += deltaTime;
    subtitle_.Update(deltaTime);

    switch (phase_)
    {
    case Phase::FadeOut:
        if (phaseTimer_ >= fadeOutTime_)
        {
            // 真っ暗になったので、ここで最初の位置に戻してもらう
            isResetRequested_ = true;
            ChangePhase(Phase::Hold);
        }
        break;

    case Phase::Hold:
        if (phaseTimer_ >= holdTime_)
        {
            ChangePhase(Phase::FadeIn);
        }
        break;

    case Phase::FadeIn:
        // 明けきる前から字幕を出す準備をし、明けきったところで無線が入る
        if (phaseTimer_ >= fadeInTime_ * 0.5f)
        {
            const float remainingFade = fadeInTime_ * 0.5f;
            ChangePhase(Phase::Show);
            phaseTimer_ = -remainingFade;
            StartSubtitle(remainingFade);
        }
        break;

    case Phase::Show:
        // 答えるボタンは、打っている途中なら全部出し、答えるボタンが出ていたら本番を始める
        if (phaseTimer_ >= skipDelay_ && GameInput::IsTriggered(GameAction::Reply))
        {
            if (!subtitle_.IsTyped())
            {
                subtitle_.SkipTyping();
            }
            else if (subtitle_.GetTypedTime() >= replyDelay_)
            {
                // 無線を切る音で答える
                if (!subtitle_.PlayClick())
                {
                    UISound::PlayConfirm();
                }
                ChangePhase(Phase::Leave);
            }
        }
        break;

    case Phase::Leave:
        if (phaseTimer_ >= leaveTime_)
        {
            isFinished_ = true;
            subtitle_.Stop();
            ChangePhase(Phase::Hidden);
        }
        break;

    case Phase::Hidden:
        break;
    }
}

void ObjectiveBanner::Draw()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    const Vector2 screenSize = { Engine::GetUIWidth(), Engine::GetUIHeight() };

    float blackoutRatio = 0.0f;
    switch (phase_)
    {
    case Phase::FadeOut:
        blackoutRatio = SmoothStep01(Ratio(phaseTimer_, fadeOutTime_));
        break;
    case Phase::Hold:
        blackoutRatio = 1.0f;
        break;
    case Phase::FadeIn:
        blackoutRatio = 1.0f - SmoothStep01(Ratio(phaseTimer_, fadeInTime_));
        break;
    case Phase::Show:
        // FadeIn の後半は Show の中で明ける
        if (phaseTimer_ < 0.0f)
        {
            blackoutRatio = 1.0f - SmoothStep01(Ratio(fadeInTime_ + phaseTimer_, fadeInTime_));
        }
        break;
    default:
        break;
    }

    float contentOpacity = 0.0f;
    if (phase_ == Phase::Show)
    {
        contentOpacity = 1.0f;
    }
    else if (phase_ == Phase::Leave)
    {
        contentOpacity = 1.0f - SmoothStep01(Ratio(phaseTimer_, leaveTime_));
    }
    if (contentOpacity > 0.0f)
    {
        subtitle_.Draw(offsetY_, contentOpacity);
        DrawReplyPrompt(contentOpacity);
    }

    if (blackoutRatio > 0.0f)
    {
        blackout_->SetSize(screenSize);
        blackout_->SetColor({ 0.0f, 0.0f, 0.0f, blackoutRatio });
        blackout_->Draw();
    }
}

void ObjectiveBanner::DrawReplyPrompt(float opacity)
{
    // 打ち終えて少ししてから、ゆっくり明滅させて出す
    const float typedTime = subtitle_.GetTypedTime();
    const float appearTime = phase_ == Phase::Leave ? 1.0f : typedTime - replyDelay_;
    if (!subtitle_.IsTyped() || appearTime <= 0.0f)
    {
        return;
    }
    const float appear = SmoothStep01(appearTime / 0.3f);
    const float pulse = 0.7f + 0.3f * std::cos(typedTime * replyPulseSpeed_);
    const float promptOpacity = opacity * appear * pulse;
    const float promptY = subtitle_.GetLastLineY(offsetY_) + replyGap_;

    replyText_->SetFontSize(replyFontSize_);
    const float textWidth = replyText_->MeasureSize().x;
    const float groupLeft = -(replyIconSize_ + replyIconGap_ + textWidth) * 0.5f;

    replyIcon_->SetTexture(GameInput::IsUsingController() ? "xbox_button_a" : "keyboard_space");
    replyIcon_->SetPosition({ groupLeft + replyIconSize_ * 0.5f, promptY });
    replyIcon_->SetSize({ replyIconSize_, replyIconSize_ });
    replyIcon_->SetColor({ 1.0f, 1.0f, 1.0f, promptOpacity });
    replyIcon_->Draw();

    replyText_->SetPosition({ groupLeft + replyIconSize_ + replyIconGap_, promptY });
    replyText_->SetColor(replyColor_);
    replyText_->SetOpacity(promptOpacity);
    replyText_->Draw();
}

void ObjectiveBanner::DebugGui()
{
    ImGui::Begin("目的の表示");

    if (ImGui::Button("表示してみる"))
    {
        Begin(false, true, 20, 180.0f);
    }

    if (ImGui::TreeNode("暗転"))
    {
        binder_->Draw("FadeOutTime", "暗くなるまで (秒)");
        binder_->Draw("HoldTime", "真っ暗なまま待つ時間 (秒)");
        binder_->Draw("FadeInTime", "明けるまで (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("文章"))
    {
        ImGui::TextDisabled("{hour} は終わりの時刻、{limit} は現実の長さ (3分 など) に置き換わる");
        binder_->Draw("Speaker", "話している人");
        binder_->Draw("Line1", "1 行目");
        binder_->Draw("Line2", "2 行目");
        binder_->Draw("Line3", "3 行目");
        binder_->Draw("ReplyLabel", "答えるボタンの文字");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("時間"))
    {
        subtitle_.DrawTimeProperties(*binder_, "");
        binder_->Draw("ReplyDelay", "打ち終えてから答えるボタンを出すまで (秒)");
        binder_->Draw("LeaveTime", "消えるまで (秒)");
        binder_->Draw("SkipDelay", "決定で飛ばせるようになるまで (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("見た目"))
    {
        subtitle_.DrawLookProperties(*binder_, "");
        binder_->Draw("OffsetY", "上端の位置 (画面の中央から)");
        binder_->Draw("ReplyGap", "最後の行から答えるボタンまで");
        binder_->Draw("ReplyFontSize", "答えるボタンの文字の大きさ");
        binder_->Draw("ReplyIconSize", "ボタンの絵の大きさ");
        binder_->Draw("ReplyIconGap", "ボタンの絵と文字の間");
        binder_->Draw("ReplyPulseSpeed", "明滅の速さ");
        binder_->Draw("ReplyColor", "答えるボタンの文字の色");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("音"))
    {
        subtitle_.DrawSoundProperties(*binder_, "");
        ImGui::TreePop();
    }

    ImGui::End();
}
