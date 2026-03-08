#pragma once

// 音声データ
struct AudioData
{
    //波形フォーマット
    WAVEFORMATEX wfex;
    // バッファの先頭アドレス
    std::vector<BYTE> buffer;
};

class MediaAudioDecoder
{
public:
    static AudioData DecodeAudioFile(const std::wstring& filePath);

private:
    Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader_;
    WAVEFORMATEX waveFormat_;
};
