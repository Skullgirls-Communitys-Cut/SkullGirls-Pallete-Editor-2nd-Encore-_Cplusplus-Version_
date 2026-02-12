#include "pch.h"
#include "UI.h"
#include "DirectXWrapper/common.h"
#include "Drawing.h"
#include "SetStyleImGui.h"
#include "DreamOrphans.cpp"
#include "PixelCode.cpp"
#include "Eyedropper/Eyedropper.h"

// Static members initialization
bool UI::s_initialized = false;
bool UI::s_visible = true;
HWND UI::s_hwnd = nullptr;
IDirect3DDevice9* UI::s_device = nullptr;
WNDPROC UI::s_originalWndProc = nullptr;

extern "C" UINT WINAPI D3D9DeviceFuncHook(UINT funcId, void* funcRef); //For callback's


auto UILogger = LOGGER::createLocal("UI", LogLevel::DEBUG_LOG);

void UI::ResizePreReset() {
    LOG_LOCAL_DEBUG(UILogger, "ResizePreReset is called!");
    if (UI::IsInitialized())
        ImGui_ImplDX9_InvalidateDeviceObjects();

}
void UI::ResizePostReset(IDirect3DDevice9* device, HRESULT res) {
    LOG_LOCAL_DEBUG(UILogger, "ResizePostReset is called!");
    if (SUCCEEDED(res) && UI::IsInitialized()) {

        ImGui_ImplDX9_CreateDeviceObjects();
            
        DWORD style = GetWindowLong(s_hwnd, GWL_STYLE);
        if ((style & WS_DLGFRAME) == 0) {
            LOG_LOCAL_DEBUG(UILogger, "Fullscreen (no 0x00400000 bit)");
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }
        else {
            LOG_LOCAL_DEBUG(UILogger, "Not fullscreen (has 0x00400000 bit)");
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }
        LOG_LOCAL_VARIABLE_HEX(UILogger, style);
    }
}
void UI::RenderPostPresent(IDirect3DDevice9* device, HRESULT res) {
    if (UI::IsInitialized() && UI::IsVisible())
        UI::Render();
}

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
    io.ConfigDpiScaleFonts |= ImGuiConfigFlags_DpiEnableScaleFonts;
    io.ConfigDpiScaleFonts |= ImGuiConfigFlags_DpiEnableScaleViewports;
    io.IniFilename = NULL;
    //io.Fonts->AddFontDefault();



    ImFont* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        PixelCode_compressed_data_base85,
        18.0f
    );

    io.FontDefault = font;
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

    //Anothes hooks for DirectXWrapper
    D3D9DeviceFuncHook(PRERESET, (void*)ResizePreReset);
    D3D9DeviceFuncHook(POSTRESET, (void*)ResizePostReset);
    D3D9DeviceFuncHook(ENDSCENE, (void*)RenderPostPresent);
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

    // Проверяем, валидно ли устройство
    if (s_device && SUCCEEDED(s_device->TestCooperativeLevel()))
    {
        BeginFrame();
        Drawing::Draw();
        EndFrame();
    }
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

    
    // Если пипетка активна, блокируем ВСЕ сообщения от мыши и клавиатуры
    if (!EyeDropper::getInstance().IsThreadFinished())
    {
        // Блокируем все сообщения мыши и клавиатуры
        if ((msg >= WM_LBUTTONDOWN && msg <= WM_MOUSELAST) ||
            (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))
        {

            return true;
        }

        // Для других сообщений продолжаем обычную обработку
        // (например, WM_PAINT, WM_SIZE и т.д.)
    }

    // Всегда передаем сообщения в ImGui для обработки
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);



    // Получаем состояние ImGui
    ImGuiIO& io = ImGui::GetIO();

    // Блокируем сообщения мыши, если ImGui хочет их обработать
    if (((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) && io.WantCaptureMouse) && s_visible)
    {
        return true;
    }

    // Блокируем сообщения клавиатуры, если ImGui хочет их обработать
    if (((msg >= WM_KEYFIRST && msg <= WM_KEYLAST) && io.WantCaptureKeyboard) && s_visible)
    {
        return true;
    }

    return CallWindowProc(s_originalWndProc, hWnd, msg, wParam, lParam);
}


bool UI::IsFullscreen()
{
    if (!s_hwnd)
        return false;

    // Проверяем стили окна
    DWORD style = GetWindowLong(s_hwnd, GWL_STYLE);

    // Если есть рамка (не полноэкранный)
    if (style & WS_BORDER || style & WS_THICKFRAME || style & WS_CAPTION)
        return false;

    if (style & WS_MINIMIZE) return true;
    // Дополнительная проверка через GetWindowPlacement
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(s_hwnd, &wp))
    {
        // Если окно развернуто и покрывает весь экран
        if (wp.showCmd == SW_SHOWMAXIMIZED)
        {
            RECT windowRect;
            GetWindowRect(s_hwnd, &windowRect);

            RECT screenRect;
            GetClientRect(GetDesktopWindow(), &screenRect);

            // Если размеры окна совпадают с размерами экрана
            if (EqualRect(&windowRect, &screenRect))
                return true;
        }
    }

    return false;
}