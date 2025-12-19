#pragma once
#include "pch.h"

namespace ImGuiCustom {

	bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags);
	bool EyeDropperButton(const char* label, const ImVec2& size = ImVec2(0, 0));
}