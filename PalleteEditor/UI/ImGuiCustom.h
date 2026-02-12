#pragma once
#include "pch.h"

namespace ImGuiCustom {

	static ImVector<ImVec4> RecentColors;

	static bool s_ColorEditFinished = false;
	static ImGuiID s_LastActiveColorEditID = 0;
	extern ImVec4 saved_palette[32];
	void AddToRecentColors(const ImVec4& color);
	// Хранилище недавних цветов
	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0);
	//bool EyeDropperButton(const char* label, const ImVec2& size = ImVec2(0, 0));
	bool ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags = 0, const float* ref_col = NULL);

}


