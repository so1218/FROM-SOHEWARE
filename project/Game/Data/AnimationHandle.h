#pragma once
#include <string>

class AnimationHandle
{
public:
    // CSVからアニメーション一覧を読み込む
    static void Initialize();

private:
    static bool initialized_;
};