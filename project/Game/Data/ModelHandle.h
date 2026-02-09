#pragma once
#include <string>

class ModelHandle
{
public:
    // CSVからモデル一覧を読み込む
    static void Initialize();

private:
    static bool initialized_;
};