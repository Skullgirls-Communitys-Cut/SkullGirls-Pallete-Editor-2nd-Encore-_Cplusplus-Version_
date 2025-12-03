#pragma once
#include "Character.h"

class PalleteFile {
	public:
	static bool LoadFromFile(Character& s_Char);
	static bool SaveToFile(const Character s_Char);
};