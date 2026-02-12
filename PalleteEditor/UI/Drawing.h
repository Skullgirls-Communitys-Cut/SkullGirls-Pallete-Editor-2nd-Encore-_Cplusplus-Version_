#pragma once
#include "pch.h"
#include "Files/GroupJSONFiles.h"
#include "UI/LoggerWidget.hpp"

class Drawing
{
private:
	inline static LPCSTR lpWindowName = "Skullgirls Pallete Editor 2nd Encore";
	inline static ImVec2 vWindowSize = { 512, 512 };
	inline static ImVec2 vFileDialogSize = { 768, 256 };
	inline static ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_MenuBar;
	inline static bool bDrawAll = true;
	inline static bool bDrawAboutWindow = false;
	inline static bool bDrawColorGroup = true;
	inline static bool bDrawSimpleColorEdit = false;
#ifdef _DEBUG
	inline static bool bDrawDevWindow = true;
	inline static bool bDrawConsole = true;
#else
	inline static bool bDrawDevWindow = false;
	inline static bool bDrawConsole = false;
#endif
	inline static LPDIRECT3DDEVICE9 pD3DDevice = nullptr;;

	inline static bool bUseColorPickerMode = false;
	inline static int m_ActiveColorIndex = 1;
private:
	inline static std::vector<ImU32> s_ClipboardColors;
	//Our draw functions, for understanding and editing
	static void DrawMenuBar();
	static void DrawAboutWindow();
	static void DrawFileDialog();
	static void DrawColorPickerWindow();

	static void DrawPaletteTabItem(); //Below Palette Tab Item Stuff ↓

	static void DrawPlayableCharactersComboBox();
	static void DrawCharacterPaletteNumSlider();
	static void DrawCharacterOptions();
	static void DrawCharacterColors();
	static void DrawColorButton(int index, ImU32 colorU32);
	static void ProcessColorChange(int index, const ImVec4& colorVec);

	static void DrawAutoLoadPaletteTabItem();
	static void DrawOptionsTabItem();
	static void CopyGroupColors(const ColorGroup& group);
	static void PasteGroupColors(const ColorGroup& group);
public:
	static void Draw();

	class Developer {
	private:
		inline static bool ShowSteamInfo = false;
		inline static bool ShowLogs = false;
		inline static bool ShowDemoWindow = false;
		inline static bool ShowTestWindow = false;
	private:

		static void DrawDevWindow();		
		static void DrawSteamInfo();
	public:
		inline static void DrawDev() {
			DrawDevWindow();
			DrawSteamInfo();
			g_logWidget->draw(&ShowLogs);
			if (ShowDemoWindow) {
				ImGui::ShowDemoWindow(&ShowDemoWindow);
			}
		}
	};
	


};