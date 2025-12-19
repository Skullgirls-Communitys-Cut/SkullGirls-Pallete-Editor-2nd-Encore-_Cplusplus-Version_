#include "pch.h"

#include "MainThread.h"
#include "ProcessManager.h"
#include "Tools/MemoryWorker.hpp"
#include "AddressesTable.h"
#include "PlayableCharactersManager.h"

#pragma region Global Variable

auto MainThreadLogger = LOGGER::createLocal("Main Thread", LogLevel::DEBUG_LOG);


namespace PatchStuff {
	std::vector<unsigned char> CodeCave = { //"Skullgirls.exe" + 332EC0
	0x60, 0x8B, 0x81, 0x30, 0x03, 0x00, 0x00, 0x8B, 0x88, 0x28, 0x03, 0x00, 0x00, 0x05, 0x30, 0x03,
	0x00, 0x00, 0x89, 0xC2, 0x83, 0xC4, 0x28, 0x58, 0x89, 0xC6, 0x83, 0xEC, 0x2C, 0x6A, 0x00, 0x6B,
	0xFF, 0x56, 0x01, 0xF8, 0x05, 0xEC, 0x06, 0x00, 0x00, 0x50, 0x89, 0xF7, 0x8B, 0x02, 0x50, 0xE8,
	0xEC, 0x49, 0xD5, 0xFF, 0x61, 0x60, 0x8B, 0x81, 0x30, 0x03, 0x00, 0x00, 0x8B, 0x88, 0x28, 0x03,
	0x00, 0x00, 0x05, 0x30, 0x03, 0x00, 0x00, 0x89, 0xC2, 0x83, 0xC4, 0x28, 0x58, 0x89, 0xC6, 0x83,
	0xEC, 0x2C, 0x6A, 0x01, 0x6B, 0xFF, 0x56, 0x01, 0xF8, 0x05, 0x54, 0x10, 0x00, 0x00, 0x50, 0x89,
	0xF7, 0x8B, 0x02, 0x50, 0xE8, 0xB7, 0x49, 0xD5, 0xFF, 0x61, 0xE9, 0x06, 0x38, 0xE5, 0xFF, 0x00
	};
	std::vector<unsigned char> JmpToCodeCave = { //"Skullgirls.exe" + 18672A
		0xE9, 0x91, 0xC7, 0x1A, 0x00 };
};

#define GAME_STATUS_MATCH_STARTED 0x4

extern std::atomic<bool> bMainStopThread;

namespace MainThread {
	std::atomic<bool> Match_Readed = false;
	std::atomic<bool> b_NODisplayChar = false;
	std::atomic<bool> b_NODisplayShadows = false;
	std::atomic<bool> b_DisplaySuperShadows = false;
}
#pragma endregion

static bool lastStateb_DisplaySuperShadows = MainThread::b_DisplaySuperShadows;
static bool lastStateb_NODisplayShadows = MainThread::b_NODisplayShadows;
static bool lastStateb_NODisplayChar = MainThread::b_NODisplayChar;

int MainThreadProc(HMODULE hModule) {
	CreateConsole();
	LOG_LOCAL_DEBUG(MainThreadLogger, "Started Main Thread!");
	LOG_LOCAL_FUNCTION_ENTRY(MainThreadLogger);
	LOG_LOCAL_THREAD_INFO(MainThreadLogger);

	LOG_LOCAL_GENERAL(MainThreadLogger, "Try to Proxy DirectX");
	if (!DirectXWrapper::Init()) {
		LOG_LOCAL_CRITICAL(MainThreadLogger, "We are not proxy d3d9.dll Why?");
		return false;
	}
	LOG_LOCAL_INFO(MainThreadLogger, "We are proxy d3d9!");


	if (!ProcessManager::instance().ReadProcess()) return -1;

	LOG_LOCAL_DEBUG(MainThreadLogger, "We are successful read Process Info:");
	LOG_LOCAL_VARIABLE_HEX(MainThreadLogger, ProcessManager::instance().s_ProcessId);
	LOG_LOCAL_VARIABLE_HEX(MainThreadLogger, ProcessManager::instance().s_BaseAddress);
	LOG_LOCAL_VARIABLE_HEX(MainThreadLogger, ProcessManager::instance().s_SG_Process);

	if (!PatchAGame()) return -1;
	LOG_LOCAL_DEBUG(MainThreadLogger, "Entering the Main Loop");
	while (!bMainStopThread) {
		MainLoop();
	}
	LOG_LOCAL_DEBUG(MainThreadLogger, "Leaving main loop. We are closed?");
	LOG_LOCAL_FUNCTION_EXIT(MainThreadLogger);
}

bool PatchAGame(){

	if (!WriteProcessMemory(
		ProcessManager::instance().s_SG_Process,
		(LPVOID)(ProcessManager::instance().s_BaseAddress + 0x332EC0),
		PatchStuff::CodeCave.data(),
		PatchStuff::CodeCave.size(),
		nullptr)) 
	{
		LOG_LOCAL_CRITICAL(MainThreadLogger, "Can't patch Code Cave");
		return false;
	}


	if (!WriteProcessMemory(
			ProcessManager::instance().s_SG_Process,
			(LPVOID)(ProcessManager::instance().s_BaseAddress + 0x18672A),
			PatchStuff::JmpToCodeCave.data(),
			PatchStuff::JmpToCodeCave.size(),
			nullptr)) 
	{
		LOG_LOCAL_CRITICAL(MainThreadLogger, "Can't patch Jump to Code Cave");
		return false;
	}
	LOG_LOCAL_DEBUG(MainThreadLogger,"We are successful patch Process");
	return true;
}

void OptionsProcessing() {

	if (MainThread::b_NODisplayChar != lastStateb_NODisplayChar) {
		LOG_LOCAL_DEBUG(MainThreadLogger, "We are patching the game for the character visibility option.");
		static const char NOPE[] = { 0x90, 0x90 };
		static const char MEM[] = { 0x77, 0x23 };

		const char* data = MainThread::b_NODisplayChar ? NOPE : MEM;

		MemoryWorker::WriteProcessMemoryWithOffsetsCached(
			ProcessManager::instance().s_SG_Process,
			ProcessManager::instance().s_BaseAddress,
			{
			static_cast<uintptr_t>(AddressTable::NEW_Base_Adress_DonotdisplayCHAR())  // Приведение к нужному типу
			},
			data,
			2,
			true);

		lastStateb_NODisplayChar = MainThread::b_NODisplayChar;
	}

	if (MainThread::b_NODisplayShadows != lastStateb_NODisplayShadows) {
		LOG_LOCAL_DEBUG(MainThreadLogger, "We are patching the game for the shadow visibility option.");
		static const char NOPE[] = { 0xEB, 0x1B };
		static const char MEM[] = { 0x75, 0x1E };

		const char* data = MainThread::b_NODisplayShadows ? NOPE : MEM;

		MemoryWorker::WriteProcessMemoryWithOffsetsCached(
			ProcessManager::instance().s_SG_Process,
			ProcessManager::instance().s_BaseAddress,
			{
			static_cast<uintptr_t>(AddressTable::NEW_Base_Adress_DonotdisplaySHADOWS())  // Приведение к нужному типу
			},
			data,
			2,
			true);

		lastStateb_NODisplayShadows = MainThread::b_NODisplayShadows;
	}

	if (MainThread::b_DisplaySuperShadows != lastStateb_DisplaySuperShadows) {
		LOG_LOCAL_DEBUG(MainThreadLogger, "We are patching the game for the super shadow option.");
		static const char NOPE[] = { 0x90, 0x90,0x90,0x90,0x90,0x90, };
		static const char MEM[] = { 0x0F, 0x8B,0x22, 0x01, 0x00, 0x00 };

		const char* data = MainThread::b_DisplaySuperShadows ? NOPE : MEM;

		MemoryWorker::WriteProcessMemoryWithOffsetsCached(
			ProcessManager::instance().s_SG_Process,
			ProcessManager::instance().s_BaseAddress,
			{
			static_cast<uintptr_t>(AddressTable::NEW_Base_Adress_Display_SuperShadowforever())  // Приведение к нужному типу
			},
			data,
			6,
			true);

		lastStateb_DisplaySuperShadows = MainThread::b_DisplaySuperShadows;
	}
}

int MainLoop() {
	int s_GameStatus;
	MemoryWorker::ReadProcessMemoryWithOffsets(
		ProcessManager::instance().s_SG_Process,
		ProcessManager::instance().s_BaseAddress, 
		{
		static_cast<uintptr_t>(AddressTable::Base_Adress()),
		static_cast<uintptr_t>(AddressTable::Offset_GameStatus())  // Приведение к нужному типу
		},
		&s_GameStatus);

	if (s_GameStatus == GAME_STATUS_MATCH_STARTED && !MainThread::Match_Readed) {
		LOG_LOCAL_INFO(MainThreadLogger, "We are start a new match");

		PlayableCharactersManager::ClearCharacterData();
		PlayableCharactersManager::RefreshCharacterSlots();
		MainThread::Match_Readed = true;

	}
	// Если мы НЕ в матче И читали персонажей (значит были в матче)
	else if (s_GameStatus != GAME_STATUS_MATCH_STARTED && MainThread::Match_Readed) {
		LOG_LOCAL_INFO(MainThreadLogger, "We are leave a match");

		PlayableCharactersManager::ClearCharacterData();
		PlayableCharactersManager::SetCurrentCharacterIndex(-1);
		MainThread::Match_Readed = false;

	}
	OptionsProcessing();

	return 0;
}
