#include "pch.h"
#include "Game.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
#ifdef _DEBUG
    D3DResourceLeakChecker leakCheck;
#endif
    Game game;
    game.Run();
	return 0;
}