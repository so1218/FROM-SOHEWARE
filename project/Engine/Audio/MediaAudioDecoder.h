#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#include <mfobjects.h>
#include <mferror.h>
#include <wrl.h>
#include <comdef.h>
#include <string>
#include <vector>

namespace FromEngine
{
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
}