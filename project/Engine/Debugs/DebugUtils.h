#pragma once

namespace FE
{

// プログラム終了時にDXGIのリソースリークをチェックするクラス
struct D3DResourceLeakChecker
{
    // デストラクタ
    ~D3DResourceLeakChecker();

    // コピーやムーブを禁止
    D3DResourceLeakChecker();
    D3DResourceLeakChecker(const D3DResourceLeakChecker&) = delete;
    D3DResourceLeakChecker& operator=(const D3DResourceLeakChecker&) = delete;
};

}