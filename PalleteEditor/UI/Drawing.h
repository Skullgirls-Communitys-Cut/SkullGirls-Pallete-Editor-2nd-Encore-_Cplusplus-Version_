#pragma once
#include "pch.h"

class Drawing
{
private:
	inline static LPCSTR lpWindowName = "Skullgirls Pallete Editor 2nd Encore";
	inline static ImVec2 vWindowSize = { 512, 512 };
	inline static ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_MenuBar;
	inline static bool bDrawAll = true;
	inline static bool bDrawAboutWindow = false;
	inline static bool bDrawAboutWindow2 = false;
	inline static LPDIRECT3DDEVICE9 pD3DDevice = nullptr;;

public:
	static void Draw();

private:
	//Our draw functions, for understanding and editing
	static void DrawMenuBar();
	static void DrawAboutWindow();

	static void DrawPaletteTabItem(); //Below Palette Tab Item Stuff ↓

	static void DrawPlayableCharactersComboBox();
	static void DrawCharacterPaletteNumSlider();
	static void DrawCharacterOptions();
	static void DrawCharacterColors();

	static void DrawAutoLoadPaletteTabItem();
};