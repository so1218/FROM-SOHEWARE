#include "MediaAudioDecoder.h"

#include <stdexcept>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")

namespace MyFrom
{
    AudioData MediaAudioDecoder::DecodeAudioFile(const std::wstring& filePath)
    {
        Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
        Microsoft::WRL::ComPtr<IMFMediaType> audioTypeOut;

        // MediaFoundation初期化
        MFStartup(MF_VERSION);

        // ソースリーダー作成
        HRESULT hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &sourceReader);
        if (FAILED(hr)) throw std::runtime_error("Failed to create source reader");

        // 出力形式をPCMに設定
        MFCreateMediaType(&audioTypeOut);
        audioTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        audioTypeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM); // LPCM
        sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioTypeOut.Get());

        // メディアタイプ取得
        Microsoft::WRL::ComPtr<IMFMediaType> nativeType;
        sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &nativeType);

        // 波形フォーマット取得
        WAVEFORMATEX* pWaveFormat = nullptr;
        UINT32 size = 0;
        MFCreateWaveFormatExFromMFMediaType(nativeType.Get(), &pWaveFormat, &size);

        std::vector<BYTE> pcmBuffer;
        DWORD streamIndex, flags;
        LONGLONG timestamp;
        Microsoft::WRL::ComPtr<IMFSample> sample;

        while (true)
        {
            hr = sourceReader->ReadSample(
                (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                0, &streamIndex, &flags, &timestamp, &sample);

            if (FAILED(hr)) break;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
            if (!sample) continue;

            // バッファを取得
            Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
            sample->ConvertToContiguousBuffer(&buffer);

            BYTE* pAudioData = nullptr;
            DWORD maxLength = 0, currentLength = 0;
            buffer->Lock(&pAudioData, &maxLength, &currentLength);

            pcmBuffer.insert(pcmBuffer.end(), pAudioData, pAudioData + currentLength);

            buffer->Unlock();
        }

        AudioData audioData{};
        audioData.wfex = *pWaveFormat;
        audioData.buffer = std::move(pcmBuffer);

        CoTaskMemFree(pWaveFormat);
        return audioData;
    }
}