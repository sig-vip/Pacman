#include "game.h"

DGLE_DYNAMIC_FUNC // Include GetEngine and FreeEngine functions to load engine library at runtime.

#define APP_CAPTION "Pacman"

#define DLL_PATH "..\\DGLE\\bin\\windows\\DGLE.dll"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	IEngineCore *pEngineCore = NULL;

	if (GetEngine(DLL_PATH, pEngineCore))
	{
		// s - standard, l - labyrinth, rl - random labyrinth mode
		Game game(pEngineCore, "s");

		//pEngineCore->ConsoleVisible(true); // show engine console (you can use "~" key to show it in any application)

		if (SUCCEEDED(pEngineCore->InitializeEngine(NULL, APP_CAPTION, TEngineWindow(SCREEN_WIDTH, SCREEN_HEIGHT, false, false, MM_4X), 15u)))
			pEngineCore->StartEngine();

		FreeEngine();
	}
	else
		MessageBox(NULL,"Couldn't load \"" DLL_PATH "\"!", APP_CAPTION, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);

	return 0;
}