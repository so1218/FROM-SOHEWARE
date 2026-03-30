#pragma once
#include <vector>
#include <windows.h>
#include <mmsystem.h>

namespace FE
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