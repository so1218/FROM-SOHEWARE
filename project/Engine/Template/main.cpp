#include "pch.h"
#include "Game.h"
#include "DebugUtils.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#ifdef _DEBUG
    // 終了時に解放し忘れた D3D12 のリソースを出力に表示する
    FS::D3DResourceLeakChecker leakCheck;
#endif
    Game game;
    game.Run();
    return 0;
}
