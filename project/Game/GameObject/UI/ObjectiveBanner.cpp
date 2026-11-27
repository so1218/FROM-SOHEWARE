#include "pch.h"
#include "ObjectiveBanner.h"
#include "UITextStyle.h"
#include "GameRandom.h"
#include "GameInput.h"
#include "TimeManager.h"
#include "AudioPlayer.h"
#include "UISound.h"
#include "UICommon.h"

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

// UTF-8 の文字数
size_t CountCharacters(const std::string& text)
{
    size_t count = 0;
    for (unsigned char byte : text)
    {
        if ((byte & 0xC0) != 0x80)
        {
            ++count;
        }
    }
    return count;
}

// 先頭から count 文字分を切り出す
std::string TakeCharacters(const std::string& text, size_t count)
{
    size_t taken = 0;
    for (size_t byteIndex = 0; byteIndex < text.size(); ++byteIndex)
    {
        if ((static_cast<unsigned char>(text[byteIndex]) & 0xC0) != 0x80)
        {
            if (taken == count)
            {
                return text.substr(0, byteIndex);
            }
            ++taken;
        }
    }
    return text;
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
    : engine_(engine)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "ObjectiveBanner");

    blackout_ = std::make_unique<Sprite>(engine_);
    blackout_->SetScreenAnchor({ 0.0f, 0.0f });
    blackout_->SetAnchorPoint({ 0.0f, 0.0f });
    blackout_->SetLayerOrder(49);

    // 映画の字幕のように、画面の中央より少し下に、上から順に並べる
    speakerText_ = std::make_unique<Text>(engine_, "Maru");
    for (std::unique_ptr<Text>& lineText : lineTexts_)
    {
        lineText = std::make_unique<Text>(engine_, "Maru");
    }
    replyText_ = std::make_unique<Text>(engine_, "Maru");
    for (Text* text : GetTexts())
    {
        UITextStyle::ApplyMaruShadow(*text);
        text->SetScreenAnchor({ 0.5f, 0.5f });
        text->SetPivot({ 0.5f, 0.0f });
        text->SetAlign(TextAlign::Center);
        text->SetLayerOrder(46);
    }
    // 答えるボタンは、絵の右に文字を置いてひとまとまりで中央にそろえる
    replyText_->SetPivot({ 0.0f, 0.5f });
    replyText_->SetAlign(TextAlign::Left);
    replyIcon_ = std::make_unique<Sprite>(engine_);
    replyIcon_->SetScreenAnchor({ 0.5f, 0.5f });
    replyIcon_->SetAnchorPoint({ 0.5f, 0.5f });
    replyIcon_->SetLayerOrder(46);
}

std::vector<Text*> ObjectiveBanner::GetTexts() const
{
    std::vector<Text*> texts = { speakerText_.get(), replyText_.get() };
    for (const std::unique_ptr<Text>& lineText : lineTexts_)
    {
        texts.push_back(lineText.get());
    }
    return texts;
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

    binder_->Bind("CharactersPerSecond", &charactersPerSecond_, 16.0f, 0.1f, 1.0f, 100.0f);
    binder_->Bind("LinePause", &linePause_, 0.4f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("CommaPause", &commaPause_, 0.12f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("PeriodPause", &periodPause_, 0.25f, 0.01f, 0.0f, 2.0f);
    binder_->Bind("ReplyDelay", &replyDelay_, 0.3f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("LeaveTime", &leaveTime_, 0.5f, 0.01f, 0.01f, 5.0f);
    binder_->Bind("SkipDelay", &skipDelay_, 0.5f, 0.01f, 0.0f, 5.0f);
    binder_->Bind("FlickerTime", &flickerTime_, 0.35f, 0.01f, 0.0f, 2.0f);

    binder_->BindFontName("Font", "Maru", [this](const std::string& fontName)
        {
            for (Text* text : GetTexts())
            {
                text->SetFont(fontName);
            }
        });
    binder_->Bind("FontSize", &fontSize_, 30.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("SpeakerFontSize", &speakerFontSize_, 20.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("OffsetY", &offsetY_, 30.0f, 1.0f);
    binder_->Bind("SpeakerGap", &speakerGap_, 34.0f, 0.5f, 0.0f, 300.0f);
    binder_->Bind("LineGap", &lineGap_, 42.0f, 0.5f, 0.0f, 300.0f);
    binder_->BindColor("TextColor", &textColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder_->BindColor("SpeakerColor", &speakerColor_, { 0.7f, 0.75f, 0.7f, 0.9f });
    binder_->Bind("ReplyGap", &replyGap_, 60.0f, 0.5f, 0.0f, 500.0f);
    binder_->Bind("ReplyFontSize", &replyFontSize_, 26.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ReplyIconSize", &replyIconSize_, 40.0f, 0.5f, 4.0f, 200.0f);
    binder_->Bind("ReplyIconGap", &replyIconGap_, 10.0f, 0.5f, 0.0f, 100.0f);
    binder_->Bind("ReplyPulseSpeed", &replyPulseSpeed_, 3.0f, 0.1f, 0.0f, 20.0f);
    binder_->BindColor("ReplyColor", &replyColor_, { 1.0f, 1.0f, 1.0f, 1.0f });

    binder_->BindString("NoiseSound", noiseSoundName_, [this](const std::string& name) { noiseSoundName_ = name; });
    binder_->Bind("NoiseVolume", &noiseVolume_, 0.35f, 0.01f, 0.0f, 1.0f);
    binder_->BindString("ClickSound", clickSoundName_, [this](const std::string& name) { clickSoundName_ = name; });
    binder_->Bind("ClickVolume", &clickVolume_, 0.6f, 0.01f, 0.0f, 1.0f);
    binder_->BindString("TypeSound", typeSoundName_, [this](const std::string& name) { typeSoundName_ = name; });
    binder_->Bind("TypeSoundVolume", &typeSoundVolume_, 0.25f, 0.01f, 0.0f, 1.0f);

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
    for (int lineIndex = 0; lineIndex < kLineCount; ++lineIndex)
    {
        lines_[lineIndex] = ApplyPlaceholders(lineSources_[lineIndex]);
        lineLengths_[lineIndex] = CountCharacters(lines_[lineIndex]);
        shownLengths_[lineIndex] = 0;
        lineTexts_[lineIndex]->SetText("");
    }
    BuildRevealTimes();
    speakerText_->SetText(speakerName_);
    replyText_->SetText(replyLabel_);
    typedTimer_ = 0.0f;
    isResetRequested_ = false;
    isFinished_ = false;
    isTypingSkipped_ = false;
    hasStartedNoise_ = false;
    StopNoise();
    ChangePhase(withFade ? Phase::FadeOut : Phase::Show);
}

void ObjectiveBanner::Cancel()
{
    isResetRequested_ = false;
    isFinished_ = false;
    StopNoise();
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

float ObjectiveBanner::GetTypingDuration() const
{
    return typingDuration_;
}

void ObjectiveBanner::BuildRevealTimes()
{
    // 1 文字ずつの出る時刻を先に決めておく
    const float interval = 1.0f / std::max(charactersPerSecond_, 0.1f);
    float time = 0.0f;
    bool hasPreviousLine = false;
    for (int lineIndex = 0; lineIndex < kLineCount; ++lineIndex)
    {
        revealTimes_[lineIndex].clear();
        const std::u32string characters = UI::DecodeUtf8(lines_[lineIndex]);
        if (characters.empty())
        {
            continue;
        }
        if (hasPreviousLine)
        {
            time += linePause_;
        }
        hasPreviousLine = true;
        for (char32_t character : characters)
        {
            time += interval;
            revealTimes_[lineIndex].push_back(time);
            if (character == U'、' || character == U'，')
            {
                time += commaPause_;
            }
            else if (character == U'。' || character == U'！' || character == U'？' || character == U'…')
            {
                time += periodPause_;
            }
        }
    }
    typingDuration_ = time;
}

void ObjectiveBanner::PlayTypeSound()
{
    AudioPlayer& audio = AudioPlayer::GetInstance();
    if (audio.HasSound(typeSoundName_))
    {
        audio.PlayUI(typeSoundName_, typeSoundVolume_);
    }
}

void ObjectiveBanner::UpdateTyping()
{
    const float typingTime = isTypingSkipped_ ? typingDuration_ : std::max(phaseTimer_, 0.0f);
    for (int lineIndex = 0; lineIndex < kLineCount; ++lineIndex)
    {
        const std::vector<float>& times = revealTimes_[lineIndex];
        const size_t count = static_cast<size_t>(std::upper_bound(times.begin(), times.end(), typingTime + 0.0001f) - times.begin());
        if (count == shownLengths_[lineIndex])
        {
            continue;
        }

        // 飛ばしたときは鳴らさない
        if (!isTypingSkipped_ && count > shownLengths_[lineIndex])
        {
            PlayTypeSound();
        }
        shownLengths_[lineIndex] = count;
        lineTexts_[lineIndex]->SetText(TakeCharacters(lines_[lineIndex], count));
    }
}

void ObjectiveBanner::StartNoise()
{
    AudioPlayer& audio = AudioPlayer::GetInstance();
    if (audio.HasSound(clickSoundName_))
    {
        audio.PlayUI(clickSoundName_, clickVolume_);
    }
    if (noiseHandle_ < 0 && audio.HasSound(noiseSoundName_))
    {
        noiseHandle_ = audio.PlayLoop(noiseSoundName_, noiseVolume_);
    }
}

void ObjectiveBanner::StopNoise()
{
    if (noiseHandle_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(noiseHandle_);
        noiseHandle_ = -1;
    }
}

void ObjectiveBanner::Update()
{
    if (phase_ == Phase::Hidden)
    {
        return;
    }

    phaseTimer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();

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
        // 明けきる前から無線が入り始める
        if (phaseTimer_ >= fadeInTime_ * 0.5f)
        {
            const float remainingFade = fadeInTime_ * 0.5f;
            ChangePhase(Phase::Show);
            phaseTimer_ = -remainingFade;
        }
        break;

    case Phase::Show:
    {
        const bool isTyped = isTypingSkipped_ || phaseTimer_ >= GetTypingDuration();
        if (phaseTimer_ >= 0.0f && !isTyped && !hasStartedNoise_)
        {
            StartNoise();
            hasStartedNoise_ = true;
        }
        UpdateTyping();

        if (isTyped)
        {
            // 声が途切れたら雑音も止める
            StopNoise();
            typedTimer_ += TimeManager::GetInstance()->GetUnscaledDeltaTime();
        }

        // 答えるボタンは、打っている途中なら全部出し、答えるボタンが出ていたら本番を始める
        if (phaseTimer_ >= skipDelay_ && GameInput::IsTriggered(GameAction::Reply))
        {
            if (!isTyped)
            {
                isTypingSkipped_ = true;
                phaseTimer_ = GetTypingDuration();
            }
            else if (typedTimer_ >= replyDelay_)
            {
                // 無線を切る音で答える
                AudioPlayer& audio = AudioPlayer::GetInstance();
                if (audio.HasSound(clickSoundName_))
                {
                    audio.PlayUI(clickSoundName_, clickVolume_);
                }
                else
                {
                    UISound::PlayConfirm();
                }
                ChangePhase(Phase::Leave);
            }
        }
        break;
    }

    case Phase::Leave:
        if (phaseTimer_ >= leaveTime_)
        {
            isFinished_ = true;
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

    if (phase_ == Phase::Show)
    {
        DrawContent(1.0f);
    }
    else if (phase_ == Phase::Leave)
    {
        DrawContent(1.0f - SmoothStep01(Ratio(phaseTimer_, leaveTime_)));
    }

    if (blackoutRatio > 0.0f)
    {
        blackout_->SetSize(screenSize);
        blackout_->SetColor({ 0.0f, 0.0f, 0.0f, blackoutRatio });
        blackout_->Draw();
    }
}

void ObjectiveBanner::DrawContent(float opacity)
{
    if (phase_ == Phase::Show && phaseTimer_ < 0.0f)
    {
        return;
    }

    // 無線が入った直後は、電波が不安定なように文字がちらつく
    float flicker = 1.0f;
    if (phase_ == Phase::Show && phaseTimer_ < flickerTime_)
    {
        flicker = GameRandom::Range(0.2f, 1.0f);
    }

    speakerText_->SetFontSize(speakerFontSize_);
    speakerText_->SetPosition({ 0.0f, offsetY_ });
    speakerText_->SetColor(speakerColor_);
    speakerText_->SetOpacity(opacity * flicker);
    speakerText_->Draw();

    // 空の行は詰める
    float lineY = offsetY_ + speakerGap_;
    for (int lineIndex = 0; lineIndex < kLineCount; ++lineIndex)
    {
        if (lineLengths_[lineIndex] == 0)
        {
            continue;
        }
        Text& lineText = *lineTexts_[lineIndex];
        lineText.SetFontSize(fontSize_);
        lineText.SetPosition({ 0.0f, lineY });
        lineText.SetColor(textColor_);
        lineText.SetOpacity(opacity * flicker);
        lineText.Draw();
        lineY += lineGap_;
    }

    DrawReplyPrompt(opacity);
}

void ObjectiveBanner::DrawReplyPrompt(float opacity)
{
    // 打ち終えて少ししてから、ゆっくり明滅させて出す
    const float appearTime = phase_ == Phase::Leave ? 1.0f : typedTimer_ - replyDelay_;
    if (appearTime <= 0.0f)
    {
        return;
    }
    const float appear = SmoothStep01(appearTime / 0.3f);
    const float pulse = 0.7f + 0.3f * std::cos(typedTimer_ * replyPulseSpeed_);
    const float promptOpacity = opacity * appear * pulse;

    float lineY = offsetY_ + speakerGap_;
    for (size_t length : lineLengths_)
    {
        if (length > 0)
        {
            lineY += lineGap_;
        }
    }
    const float promptY = lineY - lineGap_ + replyGap_;

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
        binder_->Draw("CharactersPerSecond", "1 秒に出す文字数");
        binder_->Draw("LinePause", "行と行の間 (秒)");
        binder_->Draw("CommaPause", "「、」のあとの間 (秒)");
        binder_->Draw("PeriodPause", "「。」「…」「！」のあとの間 (秒)");
        binder_->Draw("ReplyDelay", "打ち終えてから答えるボタンを出すまで (秒)");
        binder_->Draw("FlickerTime", "入った直後にちらつく長さ (秒)");
        binder_->Draw("LeaveTime", "消えるまで (秒)");
        binder_->Draw("SkipDelay", "決定で飛ばせるようになるまで (秒)");
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("見た目"))
    {
        binder_->Draw("Font", "フォント");
        binder_->Draw("FontSize", "字幕の大きさ");
        binder_->Draw("SpeakerFontSize", "話している人の大きさ");
        binder_->Draw("OffsetY", "上端の位置 (画面の中央から)");
        binder_->Draw("SpeakerGap", "話している人から 1 行目まで");
        binder_->Draw("LineGap", "行の間隔");
        binder_->Draw("TextColor", "字幕の色");
        binder_->Draw("SpeakerColor", "話している人の色");
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
        ImGui::TextDisabled("Assets/Audio にその名前のファイルがなければ鳴らさない");
        binder_->Draw("NoiseSound", "打っている間の雑音 (ループ)");
        binder_->Draw("NoiseVolume", "雑音の大きさ");
        binder_->Draw("ClickSound", "無線が入る音");
        binder_->Draw("ClickVolume", "無線が入る音の大きさ");
        binder_->Draw("TypeSound", "1 文字ごとの音");
        binder_->Draw("TypeSoundVolume", "1 文字ごとの音の大きさ");
        ImGui::TreePop();
    }

    ImGui::End();
}
