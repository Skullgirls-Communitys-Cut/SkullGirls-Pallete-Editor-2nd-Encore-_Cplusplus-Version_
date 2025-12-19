#include "pch.h"
#include "UI.h"
#include "DirectXWrapper/common.h"
#include "Drawing.h"
#include "SetStyleImGui.h"

// Static members initialization
bool UI::s_initialized = false;
bool UI::s_visible = true;
HWND UI::s_hwnd = nullptr;
IDirect3DDevice9* UI::s_device = nullptr;
WNDPROC UI::s_originalWndProc = nullptr;

auto UILogger = LOGGER::createLocal("UI", LogLevel::DEBUG_LOG);

bool UI::Initialize(HWND hwnd, IDirect3DDevice9* device)
{
    if (s_initialized)
        return true;

    if (!hwnd || !device)
        return false;

    s_hwnd = hwnd;
    s_device = device;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = NULL;
    // Set style
    //CherryTheme();
    // Initialize platform/renderer backends
    if (!ImGui_ImplWin32_Init(hwnd))
    {
        LOG_LOCAL_CRITICAL(UILogger, "Failed to initialize ImGui Win32 backend");
        return false;
    }

    if (!ImGui_ImplDX9_Init(device))
    {
        LOG_LOCAL_CRITICAL(UILogger, "Failed to initialize ImGui DX9 backend");
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    // Hook WndProc
    s_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

    s_initialized = true;
    LOG_LOCAL_INFO(UILogger, "ImGui UI initialized successfully");
    return true;
}

void UI::Shutdown()
{
    if (!s_initialized)
        return;

    // Restore original WndProc
    if (s_hwnd && s_originalWndProc)
        SetWindowLongPtr(s_hwnd, GWLP_WNDPROC, (LONG_PTR)s_originalWndProc);

    // Cleanup ImGui
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    s_initialized = false;
    s_hwnd = nullptr;
    s_device = nullptr;
    s_originalWndProc = nullptr;

    LOG_LOCAL_INFO(UILogger, "ImGui UI shutdown");
}

void UI::Render()
{
    if (!s_initialized || !s_visible)
        return;

    BeginFrame();
    Drawing::Draw();
    EndFrame();
}

void UI::BeginFrame()
{
    if (!s_initialized)
        return;

    // Start the ImGui frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UI::EndFrame()
{
    if (!s_initialized)
        return;

    // Rendering
    ImGui::EndFrame();
    ImGui::Render();

    // Actually render to DirectX9
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

LRESULT CALLBACK UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // ¬сегда передаем сообщени€ в ImGui дл€ обработки
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

    // ѕолучаем состо€ние ImGui
    ImGuiIO& io = ImGui::GetIO();

    // Ѕлокируем сообщени€ мыши, если ImGui хочет их обработать
    if (((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) && io.WantCaptureMouse) && s_visible)
    {
        return true;
    }

    // Ѕлокируем сообщени€ клавиатуры, если ImGui хочет их обработать
    if (((msg >= WM_KEYFIRST && msg <= WM_KEYLAST) && io.WantCaptureKeyboard) && s_visible)
    {
        return true;
    }

    return CallWindowProc(s_originalWndProc, hWnd, msg, wParam, lParam);
}