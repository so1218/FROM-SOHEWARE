#pragma once
#include <vector>
#include <windows.h>
#include <mmsystem.h>

// 音声データ
struct AudioData
{
    //波形フォーマット
    WAVEFORMATEX wfex;
    // バッファの先頭アドレス
    std::vector<BYTE> buffer;
};