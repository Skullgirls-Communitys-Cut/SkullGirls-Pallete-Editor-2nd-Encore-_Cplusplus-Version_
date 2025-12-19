#pragma once
int MainThreadProc(HMODULE hModule);
int MainLoop();
bool PatchAGame();

namespace MainThread {
	extern std::atomic<bool> Match_Readed;
	extern std::atomic<bool> b_NODisplayChar;
	extern std::atomic<bool> b_NODisplayShadows;
	extern std::atomic<bool> b_DisplaySuperShadows;
}