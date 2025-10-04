#include "Game.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
    D3DResourceLeakChecker leacCheck;
    Game game;
    game.Run();
	return 0;
}