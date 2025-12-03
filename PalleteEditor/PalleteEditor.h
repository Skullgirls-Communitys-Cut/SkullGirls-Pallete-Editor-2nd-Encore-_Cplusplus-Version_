#pragma once
#include "pch.h"
#include "Character.h"

class PalEdit
{
private:
	inline static DWORD s_ProcessId;
	inline static DWORD s_BaseAddress;
	inline static HANDLE s_SG_Process;
	inline static int s_GameStatus;

public:
	static int FindVectorIndexByID(int id);
	inline static int current_character_idx = -1;
	inline static std::vector<Character> Character_Vector;
	//Flags
	inline static bool bGameOpenned = false;
	inline static bool bMatchStarted = false;

	static void ChangePallete();
	static void ChangeColor(int Color_ID, __int32 colorValue);
	static void ChangeAllColors();
	static void Init();
	static void Read_Character();
};


