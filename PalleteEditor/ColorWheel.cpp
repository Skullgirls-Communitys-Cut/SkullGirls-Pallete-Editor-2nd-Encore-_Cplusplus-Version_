#include "ColorWheel.h"
#include "PalleteEditor.h"
#include "ImGui/imgui.h"
#include <string>
#include <unordered_map>
#include <cmath>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "Utills.hpp"


// per-wheel selected index (persisted by character|group key)
static std::unordered_map<std::string, int> g_selectedIndexMap;
// per-wheel layout ratio (fraction of Details width allocated to the wheel)
static std::unordered_map<std::string, float> g_wheelRatioMap;
// per-wheel left column width (swatches)
static std::unordered_map<std::string, float> g_leftWidthMap;
// dragging state: which palette index is currently being dragged per wheel
static std::unordered_map<std::string, int> g_draggingIndexMap;
// picking state for screen color picker: map wheelKey -> index being picked
static std::unordered_map<std::string, int> g_pickingIndexMap;
// waiting-for-release flag so we don't immediately capture the button click
static std::unordered_map<std::string, bool> g_pickingWaitingMap;

#ifdef _WIN32
// Low-level mouse hook & pending pick buffer so we can swallow clicks
static HHOOK g_mouseHook = NULL;
static bool g_mouseHookInitialized = false;
struct PendingPick { bool pending; POINT pt; std::string wheelKey; int index; int r; int g; int b; };
static PendingPick g_pendingPick = { false, {0,0}, std::string(), -1, 0,0,0 };
static CRITICAL_SECTION g_pickCs;

static HWND g_previewWnd = NULL;
static bool g_previewRegistered = false;

static void InstallMouseHook();
static void UninstallMouseHook();
static void EnsurePreviewWindow();
static void UpdatePreviewWindow(int x, int y, unsigned int r, unsigned int g, unsigned int b, const char* hexLabel);
static void DestroyPreviewWindow();

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && lParam) {
        MSLLHOOKSTRUCT* info = (MSLLHOOKSTRUCT*)lParam;
        if (wParam == WM_LBUTTONDOWN) {
            // find any active pick that is not waiting and capture it
            for (const auto& kv : g_pickingIndexMap) {
                const std::string& key = kv.first;
                auto wit = g_pickingWaitingMap.find(key);
                bool waiting = true;
                if (wit != g_pickingWaitingMap.end()) waiting = wit->second;
                if (!waiting) {
                    int pickIndex = kv.second;
                    HDC hdc = GetDC(NULL);
                    COLORREF cr = GetPixel(hdc, info->pt.x, info->pt.y);
                    ReleaseDC(NULL, hdc);
                    EnterCriticalSection(&g_pickCs);
                    g_pendingPick.pending = true;
                    g_pendingPick.pt = info->pt;
                    g_pendingPick.wheelKey = key;
                    g_pendingPick.index = pickIndex;
                    g_pendingPick.r = GetRValue(cr);
                    g_pendingPick.g = GetGValue(cr);
                    g_pendingPick.b = GetBValue(cr);
                    LeaveCriticalSection(&g_pickCs);

                    // swallow the click so other windows don't receive it
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

static void InstallMouseHook()
{
    if (!g_mouseHookInitialized) {
        InitializeCriticalSection(&g_pickCs);
        g_mouseHookInitialized = true;
    }
    if (!g_mouseHook) {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    }
    EnsurePreviewWindow();
}

static void UninstallMouseHook()
{
    if (g_mouseHook) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
    }
    if (g_mouseHookInitialized) {
        DeleteCriticalSection(&g_pickCs);
        g_mouseHookInitialized = false;
    }
    DestroyPreviewWindow();
}

// Simple topmost layered window to show the preview outside the app window
static LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
        // swallow mouse so clicks don't interact with this window
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void EnsurePreviewWindow()
{
    if (g_previewWnd) return;
    if (!g_previewRegistered) {
        WNDCLASS wc = {};
        wc.lpfnWndProc = PreviewWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = TEXT("SG_PickPreview");
        RegisterClass(&wc);
        g_previewRegistered = true;
    }
    g_previewWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TEXT("SG_PickPreview"), TEXT(""), WS_POPUP, 0,0, 1,1, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (g_previewWnd) {
        // make completely click-through except we will swallow clicks at hook level
        SetWindowLong(g_previewWnd, GWL_EXSTYLE, GetWindowLong(g_previewWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);
        ShowWindow(g_previewWnd, SW_SHOW);
    }
}

static void UpdatePreviewWindow(int x, int y, unsigned int r, unsigned int g, unsigned int b, const char* hexLabel)
{
    if (!g_previewWnd) return;
    const int w = 120; const int h = 28;
    HDC hdcScreen = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(hdcScreen);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hBitmap) { DeleteDC(memDC); ReleaseDC(NULL, hdcScreen); return; }
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hBitmap);
    // fill transparent
    memset(bits, 0, w * h * 4);
    // draw filled rect (white background with black text for readability)
    HBRUSH brush = CreateSolidBrush(RGB(255,255,255));
    RECT rc = {4,4, w-4, h-4};
    FillRect(memDC, &rc, brush);
    DeleteObject(brush);
    // draw border
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    HPEN oldPen = (HPEN)SelectObject(memDC, pen);
    Rectangle(memDC, 3,3, w-3, h-3);
    SelectObject(memDC, oldPen);
    DeleteObject(pen);
    // draw text hexLabel to the right (black on white background)
    SetTextColor(memDC, RGB(0,0,0));
    SetBkMode(memDC, TRANSPARENT);
    HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldF = (HFONT)SelectObject(memDC, hf);
    // position text to the right of the color square, not too far right
    RECT tr = { 36, 4, w-6, h-4 };
    DrawTextA(memDC, hexLabel, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(memDC, oldF);

    // ensure all pixels are fully opaque (set alpha byte to 255) so the layered window fully covers underlying UI
    unsigned char* pix = (unsigned char*)bits;
    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            int off = (yy * w + xx) * 4;
            pix[off + 3] = 255; // alpha byte (BGRA layout)
        }
    }

    // use UpdateLayeredWindow to show the bitmap (with per-pixel alpha set to opaque)
    POINT ptDst = { x + 16, y + 16 };
    SIZE size = { w, h };
    POINT ptSrc = {0,0};
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_previewWnd, hdcScreen, &ptDst, &size, memDC, &ptSrc, 0, &bf, ULW_ALPHA);

    // cleanup
    SelectObject(memDC, oldBmp);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, hdcScreen);
}

static void DestroyPreviewWindow()
{
    if (g_previewWnd) {
        DestroyWindow(g_previewWnd);
        g_previewWnd = NULL;
    }
}
#endif

// NOTE: color conversion helpers moved to Utills.hpp (Utills::RGBtoHSV / HSVtoRGB / ARGBToFloat4 / Float4ToARGB)

void ColorWheel::Draw(Character& currentChar, const ColorGroup& group, bool& open)
{
    std::string wheelKey = currentChar.Char_Name + std::string("|") + group.groupName;
    std::string winTitle = std::string("Color Wheel - ") + wheelKey;

    // Ensure the wheel window has a usable default size on first open and can't auto-shrink to zero
    ImGui::SetNextWindowSize(ImVec2(800, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 240), ImVec2(FLT_MAX, FLT_MAX));
    if (!ImGui::Begin(winTitle.c_str(), &open)) {
        ImGui::End();
        return;
    }

    // Main child area to contain two columns: swatches (left) and large detail canvas (right)
    ImVec2 avail = ImGui::GetContentRegionAvail();
    // Use available height so the canvas fills the window when the parent is resized
    float childHeight = avail.y > 0.0f ? avail.y : 400.0f;
    ImGui::BeginChild("WheelMain", ImVec2(0, childHeight), false);

    // Instead of columns, layout three sibling children: Swatches | Editors | Wheel
    // compute available widths and persist left/right split sizes
    ImVec2 totalAvail = ImGui::GetContentRegionAvail();
    float totalW = totalAvail.x > 0.0f ? totalAvail.x : 800.0f;
    float detailsH = childHeight;

    // left column (swatches) persisted width
    float leftW = 220.0f;
    auto lit = g_leftWidthMap.find(wheelKey);
    if (lit != g_leftWidthMap.end()) leftW = lit->second;
    if (leftW < 120.0f) leftW = 120.0f;
    if (leftW > totalW * 0.6f) leftW = totalW * 0.6f;

    // wheel width from ratio persisted
    float ratio = 0.45f;
    auto rit = g_wheelRatioMap.find(wheelKey);
    if (rit != g_wheelRatioMap.end()) ratio = rit->second;
    if (ratio < 0.2f) ratio = 0.2f; if (ratio > 0.8f) ratio = 0.8f;
    float splitterW = 6.0f;
    float wheelWidth = std::fmax(160.0f, totalW * ratio);
    float editorsWidth = totalW - leftW - wheelWidth - 2.0f * splitterW;
    if (editorsWidth < 120.0f) { editorsWidth = 120.0f; wheelWidth = totalW - leftW - editorsWidth - 2.0f * splitterW; }

    // Left column: compact swatches list
    ImGui::BeginChild("Swatches", ImVec2(leftW, childHeight), true);
    for (int i = group.startIndex; i < group.startIndex + group.count && i < (int)currentChar.Character_Colors.size(); ++i) {
        __int32 cVal = currentChar.Character_Colors[i];
        float sw[4]; Utills::ARGBToFloat4(cVal, sw);
        ImGui::PushID(i);
        ImGui::ColorButton((std::string("sw_") + std::to_string(i)).c_str(), ImVec4(sw[0], sw[1], sw[2], sw[3]), ImGuiColorEditFlags_NoAlpha, ImVec2(32, 32));
        // clicking a swatch sets the selected node for the wheel
        if (ImGui::IsItemClicked()) {
            g_selectedIndexMap[wheelKey] = i;
        }
        ImGui::SameLine();
        ImGui::Text("Idx %d", i);
        ImGui::NewLine();
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // splitter between Swatches and Editors
    ImVec2 splitterPosL = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton((std::string("split_l_") + wheelKey).c_str(), ImVec2(splitterW, detailsH));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(splitterPosL, ImVec2(splitterPosL.x + splitterW, splitterPosL.y + detailsH), IM_COL32(90,90,90,180));
    dl->AddRectFilled(ImVec2(splitterPosL.x + 2, splitterPosL.y + detailsH * 0.25f), ImVec2(splitterPosL.x + splitterW - 2, splitterPosL.y + detailsH * 0.75f), IM_COL32(140,140,140,200));
    if (ImGui::IsItemActive()) {
        float dx = ImGui::GetIO().MouseDelta.x;
        leftW += dx;

        // Ограничения leftW
        if (leftW < 120.0f) leftW = 120.0f;
        if (leftW > totalW * 0.6f) leftW = totalW * 0.6f;

        // Пересчитываем ТОЛЬКО leftW, сохраняем его
        g_leftWidthMap[wheelKey] = leftW;

        // НЕ ПЕРЕСЧИТЫВАЕМ editorsWidth и wheelWidth здесь!
        // Они пересчитаются в следующем кадре автоматически
    }
    ImGui::SameLine();
    ImGui::BeginChild("Editors", ImVec2(editorsWidth, detailsH), false);
    for (int i = group.startIndex; i < group.startIndex + group.count && i < (int)currentChar.Character_Colors.size(); ++i) {
        __int32& colorValue = currentChar.Character_Colors[i];
        // No selectable/highlight — editing widgets will set selection instead.
        float colorFloat[4] = {
            ((colorValue >> 16) & 0xFF) / 255.0f,
            ((colorValue >> 8) & 0xFF) / 255.0f,
            (colorValue & 0xFF) / 255.0f,
            ((colorValue >> 24) & 0xFF) / 255.0f
        };

        ImGui::PushID(i);
        ImGui::Text("Palette Index: %d", i);

        // Larger color editor (hide built-in numeric inputs to avoid duplicate labels)
        if (ImGui::ColorEdit4((std::string("ColorLarge##") + std::to_string(i)).c_str(), colorFloat, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
            colorValue = Utills::Float4ToARGB(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);

            // select this row when editing via color editor
            g_selectedIndexMap[wheelKey] = i;

            PalEdit::ChangeColor(i, colorValue);
            PalEdit::Read_Character();
        }

        // Numeric inputs for precise adjustment (RGB + Alpha)
        ImGui::PushItemWidth(80);
        float r = colorFloat[0], g = colorFloat[1], b = colorFloat[2], a = colorFloat[3];
        // Prefix labels so UI reads: R <value>   G <value>   B <value>   A <value>
        ImGui::Text("R"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##R") + std::to_string(i)).c_str(), &r, 0.001f, 0.0f, 1.0f)) { colorFloat[0] = r; g_selectedIndexMap[wheelKey] = i; }
        ImGui::SameLine();
        ImGui::Text("G"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##G") + std::to_string(i)).c_str(), &g, 0.001f, 0.0f, 1.0f)) { colorFloat[1] = g; g_selectedIndexMap[wheelKey] = i; }
        ImGui::SameLine();
        ImGui::Text("B"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##B") + std::to_string(i)).c_str(), &b, 0.001f, 0.0f, 1.0f)) { colorFloat[2] = b; g_selectedIndexMap[wheelKey] = i; }
        ImGui::SameLine();
        ImGui::Text("A"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##A") + std::to_string(i)).c_str(), &a, 0.001f, 0.0f, 1.0f)) { colorFloat[3] = a; g_selectedIndexMap[wheelKey] = i; }
        ImGui::SameLine();
        // Value (V) control: show as prefix label and allow vertical dragging to adjust brightness
        float hv, hs, hh;
        Utills::RGBtoHSV(colorFloat[0], colorFloat[1], colorFloat[2], hh, hs, hv);
        ImGui::Text("V"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##V") + std::to_string(i)).c_str(), &hv, 0.001f, 0.0f, 1.0f)) {
                float nr,ng,nb; Utills::HSVtoRGB(hh, hs, hv, nr, ng, nb);
            colorFloat[0] = nr; colorFloat[1] = ng; colorFloat[2] = nb;
            // compose and apply immediately
            __int32 newColor = Utills::Float4ToARGB(nr, ng, nb, colorFloat[3]);
            // selecting this index because user edited its V value
            g_selectedIndexMap[wheelKey] = i;
            PalEdit::ChangeColor(i, newColor);
            currentChar.Character_Colors[i] = newColor;
            PalEdit::Read_Character();
        }
        ImGui::PopItemWidth();

        // If any numeric changed, apply (and select the edited index)
        if (r != ((colorValue >> 16) & 0xFF) / 255.0f || g != ((colorValue >> 8) & 0xFF) / 255.0f || b != (colorValue & 0xFF) / 255.0f || a != ((colorValue >> 24) & 0xFF) / 255.0f) {
            g_selectedIndexMap[wheelKey] = i;
            colorValue = Utills::Float4ToARGB(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);
            PalEdit::ChangeColor(i, colorValue);
            PalEdit::Read_Character();
        }

        // Hex input (show as #AARRGGBB). Editable; accepts 6 (RRGGBB) or 8 (AARRGGBB) hex digits.
        {
            unsigned int a = (colorValue >> 24) & 0xFF;
            unsigned int r_byte = (colorValue >> 16) & 0xFF;
            unsigned int g_byte = (colorValue >> 8) & 0xFF;
            unsigned int b_byte = (colorValue) & 0xFF;
            char hexBuf[9]; // 8 hex chars + null
            // default show AARRGGBB
            sprintf_s(hexBuf, sizeof(hexBuf), "%02X%02X%02X%02X", a, r_byte, g_byte, b_byte);
            ImGui::Text("Hex"); ImGui::SameLine(); ImGui::Text("#"); ImGui::SameLine();
            std::string hexId = std::string("##hex_") + std::to_string(i);
            ImGuiInputTextFlags hexFlags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_EnterReturnsTrue;
            if (ImGui::InputText(hexId.c_str(), hexBuf, sizeof(hexBuf), hexFlags) || ImGui::IsItemDeactivatedAfterEdit()) {
                // parse input
                int len = (int)strlen(hexBuf);
                unsigned int newA = a, newR = r_byte, newG = g_byte, newB = b_byte;
                if (len == 8) {
                    unsigned int v = 0;
                    sscanf_s(hexBuf, "%8X", &v);
                    newA = (v >> 24) & 0xFF;
                    newR = (v >> 16) & 0xFF;
                    newG = (v >> 8) & 0xFF;
                    newB = v & 0xFF;
                } else if (len == 6) {
                    unsigned int v = 0;
                    sscanf_s(hexBuf, "%6X", &v);
                    newR = (v >> 16) & 0xFF;
                    newG = (v >> 8) & 0xFF;
                    newB = v & 0xFF;
                }
                // compose new color (preserve floats for other UI)
                __int32 newColor = (static_cast<__int32>(newA) << 24) | (static_cast<__int32>(newR) << 16) | (static_cast<__int32>(newG) << 8) | (static_cast<__int32>(newB));
                g_selectedIndexMap[wheelKey] = i;
                colorValue = newColor;
                PalEdit::ChangeColor(i, newColor);
                currentChar.Character_Colors[i] = newColor;
                PalEdit::Read_Character();
            }
        }

        // Small screen color picker button: enters global picking mode where next screen click samples a pixel
        ImGui::SameLine();
        std::string pickId = std::string("Pick##picker_") + std::to_string(i);
        if (ImGui::Button(pickId.c_str())) {
            g_pickingIndexMap[wheelKey] = i;
            g_pickingWaitingMap[wheelKey] = true; // wait for release to avoid immediate trigger
#ifdef _WIN32
            InstallMouseHook();
#endif
        }

        ImGui::PopID();
        ImGui::Separator();
    }
    ImGui::EndChild();

    // splitter - allow dragging horizontally to resize editors/wheel
    ImGui::SameLine();
    std::string splitterID = std::string("splitter_") + wheelKey;
    ImVec2 splitterPos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(splitterID.c_str(), ImVec2(splitterW, detailsH));
    ImDrawList* local_draw = ImGui::GetWindowDrawList();
    // draw visible handle
    local_draw->AddRectFilled(splitterPos, ImVec2(splitterPos.x + splitterW, splitterPos.y + detailsH), IM_COL32(90, 90, 90, 180));
    local_draw->AddRectFilled(ImVec2(splitterPos.x + 2, splitterPos.y + detailsH * 0.25f), ImVec2(splitterPos.x + splitterW - 2, splitterPos.y + detailsH * 0.75f), IM_COL32(140,140,140,200));
    bool splitterHovered = ImGui::IsItemHovered();
    bool splitterActive = ImGui::IsItemActive();
    if (splitterHovered || splitterActive) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (splitterActive) {
        // dragging left/right should change wheelWidth; moving right makes left larger (wheel smaller)
        float mouseDx = ImGui::GetIO().MouseDelta.x;
        wheelWidth -= mouseDx;
        // clamp widths
        const float minEditors = 120.0f;
        const float minWheel = 160.0f;
        if (wheelWidth < minWheel) wheelWidth = minWheel;
        if (wheelWidth > totalW - leftW - minEditors - splitterW) wheelWidth = totalW - leftW - minEditors - splitterW;
        editorsWidth = totalW - leftW - wheelWidth - 2.0f * splitterW;
        ratio = wheelWidth / totalW;
        g_wheelRatioMap[wheelKey] = ratio;
    }

    ImGui::SameLine();
    ImGui::BeginChild("WheelCanvas", ImVec2(wheelWidth, detailsH), false);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
    float canvasSize = std::fmin(canvasAvail.x, detailsH) - 24.0f;
    if (canvasSize < 64.0f) canvasSize = 64.0f;
    // center the wheel vertically in the child, align left within the child
    ImVec2 canvasCenter = ImVec2(canvasPos.x + wheelWidth * 0.5f, canvasPos.y + detailsH * 0.5f);
    float outerR = std::fmin(canvasSize * 0.45f, wheelWidth * 0.45f);
    float innerR = outerR * 0.20f;

    int selected = -1;
    auto it = g_selectedIndexMap.find(wheelKey);
    if (it != g_selectedIndexMap.end()) selected = it->second;
    if (selected < group.startIndex || selected >= group.startIndex + group.count) selected = group.startIndex;

    // Use selected V as brightness for wheel background
    float selV = 1.0f;
    if (selected >= 0 && selected < (int)currentChar.Character_Colors.size()) {
        float self[4]; Utills::ARGBToFloat4(currentChar.Character_Colors[selected], self);
        float h,s,v; Utills::RGBtoHSV(self[0], self[1], self[2], h, s, v);
        selV = v;
    }

    // Reserve interaction area for the wheel (entire child) so we can detect clicks/drags
    ImGuiIO& io = ImGui::GetIO();
    ImGui::InvisibleButton((std::string("wheel_interact_") + wheelKey).c_str(), ImVec2(wheelWidth, detailsH));
    bool wheelHovered = ImGui::IsItemHovered();
    bool wheelActive = ImGui::IsItemActive();

    // Handle global screen color picking if active for this wheel
#ifdef _WIN32
    auto pit = g_pickingIndexMap.find(wheelKey);
    if (pit != g_pickingIndexMap.end()) {
        int pickIndex = pit->second;
        bool waiting = true;
        auto wit = g_pickingWaitingMap.find(wheelKey);
        if (wit != g_pickingWaitingMap.end()) waiting = wit->second;

        // Show an on-screen preview near cursor while picking (sample continuously)
        POINT pt; GetCursorPos(&pt);
        HDC hdc = GetDC(NULL);
        COLORREF cr = GetPixel(hdc, pt.x, pt.y);
        ReleaseDC(NULL, hdc);
        unsigned int rr = GetRValue(cr);
        unsigned int gg = GetGValue(cr);
        unsigned int bb = GetBValue(cr);

        ImDrawList* fg = ImGui::GetForegroundDrawList();
        ImVec2 mpos((float)pt.x + 16.0f, (float)pt.y + 16.0f);
        ImU32 previewCol = IM_COL32((int)rr, (int)gg, (int)bb, 255);
        fg->AddRectFilled(mpos, ImVec2(mpos.x + 24.0f, mpos.y + 24.0f), previewCol);
        char hexLabel[16]; sprintf_s(hexLabel, sizeof(hexLabel), "#%02X%02X%02X", rr, gg, bb);
        fg->AddRect(mpos, ImVec2(mpos.x + 24.0f, mpos.y + 24.0f), IM_COL32(0,0,0,180));
        fg->AddText(ImGui::GetFont(), ImGui::GetFontSize()*0.9f, ImVec2(mpos.x + 28.0f, mpos.y + 4.0f), IM_COL32(255,255,255,230), hexLabel);
    #ifdef _WIN32
        UpdatePreviewWindow(pt.x, pt.y, rr, gg, bb, hexLabel);
    #endif

        // If the hook captured a click, it fills g_pendingPick; consume it here and apply without letting other windows receive the click
        bool hasPending = false;
        PendingPick copyPick;
        EnterCriticalSection(&g_pickCs);
        hasPending = g_pendingPick.pending;
        if (hasPending) {
            copyPick = g_pendingPick;
            g_pendingPick.pending = false;
        }
        LeaveCriticalSection(&g_pickCs);

        if (hasPending) {
            unsigned int a = (currentChar.Character_Colors[copyPick.index] >> 24) & 0xFF;
            __int32 newColor = (static_cast<__int32>(a) << 24) | (static_cast<__int32>(copyPick.r) << 16) | (static_cast<__int32>(copyPick.g) << 8) | (static_cast<__int32>(copyPick.b));
            g_selectedIndexMap[wheelKey] = copyPick.index;
            PalEdit::ChangeColor(copyPick.index, newColor);
            currentChar.Character_Colors[copyPick.index] = newColor;
            PalEdit::Read_Character();

            // clear picking state for that wheel and uninstall hook
            g_pickingIndexMap.erase(copyPick.wheelKey);
            g_pickingWaitingMap.erase(copyPick.wheelKey);
            UninstallMouseHook();
        } else {
            // if waiting for release, clear the waiting flag on LBUTTON up (same behavior as before)
            SHORT vk = GetAsyncKeyState(VK_LBUTTON);
            bool down = (vk & 0x8000) != 0;
            if (waiting) {
                if (!down) {
                    g_pickingWaitingMap[wheelKey] = false;
                    waiting = false;
                }
            }
        }
    }
#endif

    // Draw hue-saturation disk (approximate by many quads)
    const int segments = 128;
    for (int si = 0; si < segments; ++si) {
        float a0 = ((float)si / (float)segments) * 2.0f * 3.14159265f;
        float a1 = ((float)(si+1) / (float)segments) * 2.0f * 3.14159265f;
        ImVec2 p0 = ImVec2(canvasCenter.x + outerR * cosf(a0), canvasCenter.y + outerR * sinf(a0));
        ImVec2 p1 = ImVec2(canvasCenter.x + outerR * cosf(a1), canvasCenter.y + outerR * sinf(a1));
        ImVec2 q0 = ImVec2(canvasCenter.x + innerR * cosf(a0), canvasCenter.y + innerR * sinf(a0));
        ImVec2 q1 = ImVec2(canvasCenter.x + innerR * cosf(a1), canvasCenter.y + innerR * sinf(a1));
        float hue = (float)si / (float)segments * 360.0f;
        float rr,gg,bb; Utills::HSVtoRGB(hue, 1.0f, selV, rr, gg, bb);
        int col = IM_COL32((int)(rr*255), (int)(gg*255), (int)(bb*255), 255);
        ImVec2 poly[4] = { p0, p1, q1, q0 };
        draw_list->AddConvexPolyFilled(poly, 4, col);
    }
    // inner center fill to smooth
    draw_list->AddCircleFilled(canvasCenter, innerR, IM_COL32(30,30,30,220), 64);

    // draw nodes mapped by hue and saturation
    int nodeRadius = 8;
    for (int idx = 0; idx < group.count && (group.startIndex + idx) < (int)currentChar.Character_Colors.size(); ++idx) {
        int paletteIndex = group.startIndex + idx;
        float cf[4]; Utills::ARGBToFloat4(currentChar.Character_Colors[paletteIndex], cf);
        float h,s,v; Utills::RGBtoHSV(cf[0], cf[1], cf[2], h, s, v);
        float angle = (h / 360.0f) * 2.0f * 3.14159265f;
        float r = innerR + (outerR - innerR) * s;
        ImVec2 pos = ImVec2(canvasCenter.x + r * cosf(angle), canvasCenter.y + r * sinf(angle));
        int col = IM_COL32((int)(cf[0]*255), (int)(cf[1]*255), (int)(cf[2]*255), (int)(cf[3]*255));
        // draw a subtle dark outline for contrast
        draw_list->AddCircle(pos, (float)nodeRadius + 1.0f, IM_COL32(20,20,20,220), 16, 1.5f);
        draw_list->AddCircleFilled(pos, (float)nodeRadius, col, 16);
        // highlight selected with a brighter white ring
        if (paletteIndex == selected) {
            draw_list->AddCircle(pos, 10.0f, IM_COL32(255,255,255,200), 16, 2.0f);
        }

        // Handle mouse interactions: start drag on click, continue while mouse down
        // Only consider interactions when user clicks within nodeRadius of the node
        if (wheelHovered && ImGui::IsMouseClicked(0)) {
            ImVec2 mp = io.MousePos;
            float dx = mp.x - pos.x; float dy = mp.y - pos.y;
            if ((dx*dx + dy*dy) <= (nodeRadius+4)*(nodeRadius+4)) {
                // select this node
                g_selectedIndexMap[wheelKey] = paletteIndex;
                // begin dragging this node immediately
                g_draggingIndexMap[wheelKey] = paletteIndex;
            }
        }
        int draggingIndex = -1;
        auto dit = g_draggingIndexMap.find(wheelKey);
        if (dit != g_draggingIndexMap.end()) draggingIndex = dit->second;
        if (draggingIndex == paletteIndex && io.MouseDown[0]) {
            // compute new hue/sat from mouse position
            ImVec2 mp = io.MousePos;
            float dx = mp.x - canvasCenter.x; float dy = mp.y - canvasCenter.y;
            float dist = sqrtf(dx*dx + dy*dy);
            float newSat = 0.0f;
            if (dist <= innerR) newSat = 0.0f;
            else newSat = (dist - innerR) / (outerR - innerR);
            if (newSat < 0.0f) newSat = 0.0f; if (newSat > 1.0f) newSat = 1.0f;
            float newHue = atan2f(dy, dx) * (180.0f / 3.14159265f);
            if (newHue < 0.0f) newHue += 360.0f;

            // preserve original value (v) and alpha
            float orig[4]; Utills::ARGBToFloat4(currentChar.Character_Colors[paletteIndex], orig);
            float oh,os,ov; Utills::RGBtoHSV(orig[0], orig[1], orig[2], oh, os, ov);
            float nr,ng,nb; Utills::HSVtoRGB(newHue, newSat, ov, nr, ng, nb);
            __int32 newColor = Utills::Float4ToARGB(nr, ng, nb, orig[3]);
            // write immediately
            PalEdit::ChangeColor(paletteIndex, newColor);
            // update local copy so UI reflects change immediately
            currentChar.Character_Colors[paletteIndex] = newColor;
            PalEdit::Read_Character();
        }
        // stop dragging on mouse release
        if (!io.MouseDown[0]) {
            auto dit2 = g_draggingIndexMap.find(wheelKey);
            if (dit2 != g_draggingIndexMap.end()) g_draggingIndexMap.erase(dit2);
        }
    }

    ImGui::EndChild();
    // end WheelMain child
    ImGui::EndChild();
    ImGui::End();
}
