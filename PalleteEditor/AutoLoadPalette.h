#pragma once
#include <vector>
#include <string>

struct Auto_Pal
{
	std::string CharName;
	int PalNum;
	std::string PalPath;
};

class AutoPalette {
public:
	static inline bool b_LoadOnlyMySide = true;

	static void init();
	static void save();
	static void load();
	static inline std::vector<Auto_Pal> Auto_Pals;

};