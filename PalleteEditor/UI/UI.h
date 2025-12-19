#pragma once
#include "pch.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class UI
{
public:
    static bool Initialize(HWND hwnd, IDirect3DDevice9* device);
    static void Shutdown();
    static void Render();
    static void BeginFrame();
    static void EndFrame();
    static bool IsInitialized() { return s_initialized; }

    static void ToggleVisibility() { s_visible = !s_visible; }
    static bool IsVisible() { return s_visible; }
    static void SetVisible(bool visible) { s_visible = visible; }

    //static void SetStyleColors();

private:
    static bool s_initialized;
    static bool s_visible;
    static HWND s_hwnd;
    static IDirect3DDevice9* s_device;

    // WndProc hook
    static WNDPROC s_originalWndProc;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};