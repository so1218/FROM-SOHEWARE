#pragma once
#include <vector>
#include <windows.h>
#include <mmsystem.h>

namespace FS
{

// 音声データ
struct AudioData
{
    //波形フォーマット
    WAVEFORMATEX wfex;
    // バッファの先頭アドレス
    std::vector<BYTE> buffer;
};

}