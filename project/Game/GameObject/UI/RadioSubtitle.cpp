#include "pch.h"
#include "RadioSubtitle.h"
#include "UITextStyle.h"
#include "UICommon.h"
#include "GameRandom.h"
#include "AudioPlayer.h"

using namespace FS;

namespace
{
// UTF-8 の文字数 (バイト数ではない)
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
}

RadioSubtitle::RadioSubtitle(Engine* engine)
    : engine_(engine)
{
    // 映画の字幕のように、上から順に中央にそろえて並べる
    speakerText_ = std::make_unique<Text>(engine_, "Maru");
    for (std::unique_ptr<Text>& lineText : lineTexts_)
    {
        lineText = std::make_unique<Text>(engine_, "Maru");
    }
    for (Text* text : GetTexts())
    {
        UITextStyle::ApplyMaruShadow(*text);
        text->SetScreenAnchor({ 0.5f, 0.5f });
        text->SetPivot({ 0.5f, 0.0f });
        text->SetAlign(TextAlign::Center);
        text->SetLayerOrder(46);
    }
}

RadioSubtitle::~RadioSubtitle()
{
    StopNoise();
}

std::vector<Text*> RadioSubtitle::GetTexts()
{
    std::vector<Text*> texts = { speakerText_.get() };
    for (std::unique_ptr<Text>& lineText : lineTexts_)
    {
        texts.push_back(lineText.get());
    }
    return texts;
}

void RadioSubtitle::BindProperties(PropertyBinder& binder, const std::string& prefix)
{
    binder.Bind(prefix + "CharactersPerSecond", &charactersPerSecond_, 16.0f, 0.1f, 1.0f, 100.0f);
    binder.Bind(prefix + "LinePause", &linePause_, 0.4f, 0.01f, 0.0f, 5.0f);
    binder.Bind(prefix + "CommaPause", &commaPause_, 0.12f, 0.01f, 0.0f, 2.0f);
    binder.Bind(prefix + "PeriodPause", &periodPause_, 0.25f, 0.01f, 0.0f, 2.0f);
    binder.Bind(prefix + "FlickerTime", &flickerTime_, 0.35f, 0.01f, 0.0f, 2.0f);

    binder.BindFontName(prefix + "Font", "Maru", [this](const std::string& fontName)
        {
            for (Text* text : GetTexts())
            {
                text->SetFont(fontName);
            }
        });
    binder.Bind(prefix + "FontSize", &fontSize_, 30.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind(prefix + "SpeakerFontSize", &speakerFontSize_, 20.0f, 0.5f, 4.0f, 200.0f);
    binder.Bind(prefix + "SpeakerGap", &speakerGap_, 34.0f, 0.5f, 0.0f, 300.0f);
    binder.Bind(prefix + "LineGap", &lineGap_, 42.0f, 0.5f, 0.0f, 300.0f);
    binder.BindColor(prefix + "TextColor", &textColor_, { 1.0f, 1.0f, 1.0f, 1.0f });
    binder.BindColor(prefix + "SpeakerColor", &speakerColor_, { 0.7f, 0.75f, 0.7f, 0.9f });

    binder.BindString(prefix + "NoiseSound", noiseSoundName_, [this](const std::string& name) { noiseSoundName_ = name; });
    binder.Bind(prefix + "NoiseVolume", &noiseVolume_, 0.35f, 0.01f, 0.0f, 1.0f);
    binder.BindString(prefix + "ClickSound", clickSoundName_, [this](const std::string& name) { clickSoundName_ = name; });
    binder.Bind(prefix + "ClickVolume", &clickVolume_, 0.6f, 0.01f, 0.0f, 1.0f);
    binder.BindString(prefix + "TypeSound", typeSoundName_, [this](const std::string& name) { typeSoundName_ = name; });
    binder.Bind(prefix + "TypeSoundVolume", &typeSoundVolume_, 0.25f, 0.01f, 0.0f, 1.0f);
}

void RadioSubtitle::DrawTimeProperties(PropertyBinder& binder, const std::string& prefix)
{
    binder.Draw(prefix + "CharactersPerSecond", "1 秒に出す文字数");
    binder.Draw(prefix + "LinePause", "行と行の間 (秒)");
    binder.Draw(prefix + "CommaPause", "「、」のあとの間 (秒)");
    binder.Draw(prefix + "PeriodPause", "「。」「…」「！」のあとの間 (秒)");
    binder.Draw(prefix + "FlickerTime", "入った直後にちらつく長さ (秒)");
}

void RadioSubtitle::DrawLookProperties(PropertyBinder& binder, const std::string& prefix)
{
    binder.Draw(prefix + "Font", "フォント");
    binder.Draw(prefix + "FontSize", "字幕の大きさ");
    binder.Draw(prefix + "SpeakerFontSize", "話している人の大きさ");
    binder.Draw(prefix + "SpeakerGap", "話している人から 1 行目まで");
    binder.Draw(prefix + "LineGap", "行の間隔");
    binder.Draw(prefix + "TextColor", "字幕の色");
    binder.Draw(prefix + "SpeakerColor", "話している人の色");
}

void RadioSubtitle::DrawSoundProperties(PropertyBinder& binder, const std::string& prefix)
{
    ImGui::TextDisabled("Assets/Audio にその名前のファイルがなければ鳴らさない");
    binder.Draw(prefix + "NoiseSound", "打っている間の雑音 (ループ)");
    binder.Draw(prefix + "NoiseVolume", "雑音の大きさ");
    binder.Draw(prefix + "ClickSound", "無線が入る音");
    binder.Draw(prefix + "ClickVolume", "無線が入る音の大きさ");
    binder.Draw(prefix + "TypeSound", "1 文字ごとの音");
    binder.Draw(prefix + "TypeSoundVolume", "1 文字ごとの音の大きさ");
}

void RadioSubtitle::Start(const std::string& speaker, const std::vector<std::string>& lines, float startDelay)
{
    StopNoise();
    speakerText_->SetText(speaker);
    for (int lineIndex = 0; lineIndex < kMaxLineCount; ++lineIndex)
    {
        lines_[lineIndex] = lineIndex < static_cast<int>(lines.size()) ? lines[lineIndex] : std::string();
        lineLengths_[lineIndex] = CountCharacters(lines_[lineIndex]);
        shownLengths_[lineIndex] = 0;
        lineTexts_[lineIndex]->SetText("");
    }
    BuildRevealTimes();
    isActive_ = true;
    timer_ = -std::max(startDelay, 0.0f);
    typedTimer_ = 0.0f;
    isTypingSkipped_ = false;
    hasStartedNoise_ = false;
}

void RadioSubtitle::Stop()
{
    StopNoise();
    isActive_ = false;
}

bool RadioSubtitle::IsTyped() const
{
    return isActive_ && (isTypingSkipped_ || timer_ >= typingDuration_);
}

void RadioSubtitle::SkipTyping()
{
    if (!isActive_ || IsTyped())
    {
        return;
    }
    isTypingSkipped_ = true;
    timer_ = std::max(timer_, typingDuration_);
}

void RadioSubtitle::Update(float deltaTime)
{
    if (!isActive_)
    {
        return;
    }
    timer_ += deltaTime;

    const bool isTyped = IsTyped();
    if (timer_ >= 0.0f && !isTyped && !hasStartedNoise_)
    {
        StartNoise();
        hasStartedNoise_ = true;
    }
    UpdateTyping();

    if (isTyped)
    {
        // 声が途切れたら雑音も止める
        StopNoise();
        typedTimer_ += deltaTime;
    }
}

void RadioSubtitle::BuildRevealTimes()
{
    // 1 文字ずつの出る時刻を先に決めておく。句読点のあとは少し間を空けて、話している息づかいを出す
    const float interval = 1.0f / std::max(charactersPerSecond_, 0.1f);
    float time = 0.0f;
    bool hasPreviousLine = false;
    for (int lineIndex = 0; lineIndex < kMaxLineCount; ++lineIndex)
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

void RadioSubtitle::UpdateTyping()
{
    const float typingTime = isTypingSkipped_ ? typingDuration_ : std::max(timer_, 0.0f);
    for (int lineIndex = 0; lineIndex < kMaxLineCount; ++lineIndex)
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
            AudioPlayer& audio = AudioPlayer::GetInstance();
            if (audio.HasSound(typeSoundName_))
            {
                audio.PlayUI(typeSoundName_, typeSoundVolume_);
            }
        }
        shownLengths_[lineIndex] = count;
        lineTexts_[lineIndex]->SetText(TakeCharacters(lines_[lineIndex], count));
    }
}

bool RadioSubtitle::PlayClick() const
{
    AudioPlayer& audio = AudioPlayer::GetInstance();
    if (!audio.HasSound(clickSoundName_))
    {
        return false;
    }
    audio.PlayUI(clickSoundName_, clickVolume_);
    return true;
}

void RadioSubtitle::StartNoise()
{
    PlayClick();
    AudioPlayer& audio = AudioPlayer::GetInstance();
    if (noiseHandle_ < 0 && audio.HasSound(noiseSoundName_))
    {
        noiseHandle_ = audio.PlayLoop(noiseSoundName_, noiseVolume_);
    }
}

void RadioSubtitle::StopNoise()
{
    if (noiseHandle_ >= 0)
    {
        AudioPlayer::GetInstance().Stop(noiseHandle_);
        noiseHandle_ = -1;
    }
}

float RadioSubtitle::GetLastLineY(float topY) const
{
    float lineY = topY + speakerGap_;
    bool hasLine = false;
    for (size_t length : lineLengths_)
    {
        if (length > 0)
        {
            lineY += hasLine ? lineGap_ : 0.0f;
            hasLine = true;
        }
    }
    return lineY;
}

void RadioSubtitle::Draw(float topY, float opacity)
{
    if (!IsStarted() || opacity <= 0.0f)
    {
        return;
    }

    // 無線が入った直後は、電波が不安定なように文字がちらつく
    float flicker = 1.0f;
    if (timer_ < flickerTime_ && !isTypingSkipped_)
    {
        flicker = GameRandom::Range(0.2f, 1.0f);
    }

    speakerText_->SetFontSize(speakerFontSize_);
    speakerText_->SetPosition({ 0.0f, topY });
    speakerText_->SetColor(speakerColor_);
    speakerText_->SetOpacity(opacity * flicker);
    speakerText_->Draw();

    // 空の行は詰める
    float lineY = topY + speakerGap_;
    for (int lineIndex = 0; lineIndex < kMaxLineCount; ++lineIndex)
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
}
