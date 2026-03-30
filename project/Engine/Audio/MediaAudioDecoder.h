#pragma once
#include "AudioDefinition.h"

namespace FE
{

class MediaAudioDecoder
{
public:
    static AudioData DecodeAudioFile(const std::wstring& filePath);

private:
    Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader_;
    WAVEFORMATEX waveFormat_;
};

}

