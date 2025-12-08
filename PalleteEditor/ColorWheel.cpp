#define NOMINMAX
#include "ColorWheel.h"
#include "PalleteEditor.h"
#include "ImGui/imgui.h"
#include <string>
#include <unordered_map>
#include <cmath>
#include <chrono>

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
// picking phase: 0=not picking, 1=waiting for release, 2=actively picking
static std::unordered_map<std::string, int> g_pickingPhaseMap;
// last pick position for preview
static std::unordered_map<std::string, POINT> g_lastPickPosMap;
// last picked color for preview
static std::unordered_map<std::string, COLORREF> g_lastPickColorMap;

// Fast pixel sampling without GDI object creation/destruction
class FastPixelSampler {
public:
    FastPixelSampler() {
        hdcScreen_ = GetDC(NULL);
        memset(&bmi_, 0, sizeof(BITMAPINFO));
        bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi_.bmiHeader.biWidth = 1;
        bmi_.bmiHeader.biHeight = -1;
        bmi_.bmiHeader.biPlanes = 1;
        bmi_.bmiHeader.biBitCount = 32;
        bmi_.bmiHeader.biCompression = BI_RGB;

        hBitmap_ = CreateDIBSection(hdcScreen_, &bmi_, DIB_RGB_COLORS, &pixelData_, NULL, 0);
        hdcMem_ = CreateCompatibleDC(hdcScreen_);
        oldBitmap_ = (HBITMAP)SelectObject(hdcMem_, hBitmap_);
    }

    ~FastPixelSampler() {
        if (hdcMem_) {
            SelectObject(hdcMem_, oldBitmap_);
            DeleteDC(hdcMem_);
        }
        if (hBitmap_) DeleteObject(hBitmap_);
        if (hdcScreen_) ReleaseDC(NULL, hdcScreen_);
    }

    COLORREF GetPixelFast(int x, int y) {
        if (!hdcScreen_ || !hdcMem_) return 0;

        // Быстрое копирование одного пикселя
        BitBlt(hdcMem_, 0, 0, 1, 1, hdcScreen_, x, y, SRCCOPY);

        // Получаем цвет без вызова GetDIBits
        if (pixelData_) {
            return *((DWORD*)pixelData_) & 0x00FFFFFF;
        }
        return 0;
    }

private:
    HDC hdcScreen_ = NULL;
    HDC hdcMem_ = NULL;
    HBITMAP hBitmap_ = NULL;
    HBITMAP oldBitmap_ = NULL;
    BITMAPINFO bmi_;
    void* pixelData_ = NULL;
};

static FastPixelSampler g_pixelSampler;

// Функция обновления превью пикера
static void DrawColorPickerPreview(const std::string& wheelKey) {
    static auto lastUpdateTime = std::chrono::steady_clock::now();

    auto itPhase = g_pickingPhaseMap.find(wheelKey);
    if (itPhase == g_pickingPhaseMap.end() || itPhase->second != 2) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime);

    // Обновляем не чаще чем 30 FPS
    if (elapsed.count() < 33) {
        return;
    }
    lastUpdateTime = now;

    POINT pt;
    GetCursorPos(&pt);

    COLORREF cr = g_pixelSampler.GetPixelFast(pt.x, pt.y);

    // Сохраняем для быстрого доступа
    g_lastPickPosMap[wheelKey] = pt;
    g_lastPickColorMap[wheelKey] = cr;

    // Отрисовываем превью
    if (ImGui::Begin("Color Pick Preview", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove)) {

        ImGui::SetWindowPos(ImVec2((float)pt.x + 16.0f, (float)pt.y + 16.0f));
        ImVec4 color(GetRValue(cr) / 255.0f, GetGValue(cr) / 255.0f, GetBValue(cr) / 255.0f, 1.0f);

        ImGui::ColorButton("##preview", color,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
            ImVec2(24, 24));
        ImGui::SameLine();
        char hexLabel[16];
        sprintf_s(hexLabel, sizeof(hexLabel), "#%02X%02X%02X",
            GetRValue(cr), GetGValue(cr), GetBValue(cr));
        ImGui::Text("%s", hexLabel);
        ImGui::End();
    }
}

// Функция обработки пикера
static void ProcessColorPicker(Character& currentChar, const std::string& wheelKey) {
    auto itPhase = g_pickingPhaseMap.find(wheelKey);
    if (itPhase == g_pickingPhaseMap.end()) {
        return;
    }

    int phase = itPhase->second;
    auto itIndex = g_pickingIndexMap.find(wheelKey);
    if (itIndex == g_pickingIndexMap.end()) {
        return;
    }

    int pickIndex = itIndex->second;

    switch (phase) {
    case 1: // Ждем отпускания кнопки
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            // Кнопка отпущена, переходим к активному пикингу
            g_pickingPhaseMap[wheelKey] = 2;
        }
        break;

    case 2: // Активный пикинг
    {
        // Показываем превью
        DrawColorPickerPreview(wheelKey);

        // Проверяем клик для выбора цвета
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Получаем текущую позицию курсора
            auto itPos = g_lastPickPosMap.find(wheelKey);
            auto itColor = g_lastPickColorMap.find(wheelKey);

            if (itPos != g_lastPickPosMap.end() && itColor != g_lastPickColorMap.end()) {
                POINT pt = itPos->second;
                COLORREF cr = itColor->second;

                // Применяем цвет
                unsigned int a = (currentChar.Character_Colors[pickIndex] >> 24) & 0xFF;
                __int32 newColor = (static_cast<__int32>(a) << 24) |
                    (static_cast<__int32>(GetRValue(cr)) << 16) |
                    (static_cast<__int32>(GetGValue(cr)) << 8) |
                    (static_cast<__int32>(GetBValue(cr)));

                g_selectedIndexMap[wheelKey] = pickIndex;
                PalEdit::ChangeColor(pickIndex, newColor);
                currentChar.Character_Colors[pickIndex] = newColor;
                PalEdit::Read_Character();

                // Завершаем пикинг
                g_pickingIndexMap.erase(wheelKey);
                g_pickingPhaseMap.erase(wheelKey);
                g_lastPickPosMap.erase(wheelKey);
                g_lastPickColorMap.erase(wheelKey);
            }
        }

        // Проверяем отмену по правой кнопке или Escape
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            // Отменяем пикинг
            g_pickingIndexMap.erase(wheelKey);
            g_pickingPhaseMap.erase(wheelKey);
            g_lastPickPosMap.erase(wheelKey);
            g_lastPickColorMap.erase(wheelKey);
        }
        break;
    }
    }
}

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

    // Main child area
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float childHeight = avail.y > 0.0f ? avail.y : 400.0f;
    ImGui::BeginChild("WheelMain", ImVec2(0, childHeight), false);

    // Layout calculation
    ImVec2 totalAvail = ImGui::GetContentRegionAvail();
    float totalW = totalAvail.x > 0.0f ? totalAvail.x : 800.0f;
    float detailsH = childHeight;

    // Left column width
    float leftW = 220.0f;
    auto lit = g_leftWidthMap.find(wheelKey);
    if (lit != g_leftWidthMap.end()) leftW = lit->second;
    leftW = std::max(120.0f, std::min(leftW, totalW * 0.6f));

    // Wheel width from ratio
    float ratio = 0.45f;
    auto rit = g_wheelRatioMap.find(wheelKey);
    if (rit != g_wheelRatioMap.end()) ratio = rit->second;
    ratio = std::max(0.2f, std::min(ratio, 0.8f));

    float splitterW = 6.0f;
    float wheelWidth = std::max(160.0f, totalW * ratio);
    float editorsWidth = totalW - leftW - wheelWidth - 2.0f * splitterW;

    if (editorsWidth < 120.0f) {
        editorsWidth = 120.0f;
        wheelWidth = totalW - leftW - editorsWidth - 2.0f * splitterW;
    }

    // Left column: compact swatches list
    ImGui::BeginChild("Swatches", ImVec2(leftW, childHeight), true);
    for (int i = group.startIndex; i < group.startIndex + group.count && i < (int)currentChar.Character_Colors.size(); ++i) {
        __int32 cVal = currentChar.Character_Colors[i];
        float sw[4]; Utills::ARGBToFloat4(cVal, sw);
        ImGui::PushID(i);
        ImGui::ColorButton((std::string("sw_") + std::to_string(i)).c_str(),
            ImVec4(sw[0], sw[1], sw[2], sw[3]),
            ImGuiColorEditFlags_NoAlpha, ImVec2(32, 32));

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

    // Splitter between Swatches and Editors
    ImVec2 splitterPosL = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton((std::string("split_l_") + wheelKey).c_str(), ImVec2(splitterW, detailsH));

    if (ImGui::IsItemActive()) {
        float dx = ImGui::GetIO().MouseDelta.x;
        leftW += dx;
        leftW = std::max(120.0f, std::min(leftW, totalW * 0.6f));
        g_leftWidthMap[wheelKey] = leftW;
    }

    // Draw splitter
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(splitterPosL, ImVec2(splitterPosL.x + splitterW, splitterPosL.y + detailsH), IM_COL32(90, 90, 90, 180));
    dl->AddRectFilled(ImVec2(splitterPosL.x + 2, splitterPosL.y + detailsH * 0.25f),
        ImVec2(splitterPosL.x + splitterW - 2, splitterPosL.y + detailsH * 0.75f),
        IM_COL32(140, 140, 140, 200));

    ImGui::SameLine();

    // Editors column
    ImGui::BeginChild("Editors", ImVec2(editorsWidth, detailsH), false);
    for (int i = group.startIndex; i < group.startIndex + group.count && i < (int)currentChar.Character_Colors.size(); ++i) {
        __int32& colorValue = currentChar.Character_Colors[i];

        // Pre-calculate float values once
        float colorFloat[4] = {
            ((colorValue >> 16) & 0xFF) / 255.0f,
            ((colorValue >> 8) & 0xFF) / 255.0f,
            (colorValue & 0xFF) / 255.0f,
            ((colorValue >> 24) & 0xFF) / 255.0f
        };

        ImGui::PushID(i);
        ImGui::Text("Palette Index: %d", i);

        // Color editor
        bool colorChanged = ImGui::ColorEdit4((std::string("ColorLarge##") + std::to_string(i)).c_str(),
            colorFloat,
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_NoLabel);

        if (colorChanged) {
            colorValue = Utills::Float4ToARGB(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);
            g_selectedIndexMap[wheelKey] = i;
            PalEdit::ChangeColor(i, colorValue);
            PalEdit::Read_Character();
        }

        // Numeric inputs
        ImGui::PushItemWidth(80);
        float r = colorFloat[0], g = colorFloat[1], b = colorFloat[2], a = colorFloat[3];

        // RGB+A controls
        bool rgbChanged = false;
        ImGui::Text("R"); ImGui::SameLine();
        rgbChanged |= ImGui::DragFloat((std::string("##R") + std::to_string(i)).c_str(), &r, 0.001f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::Text("G"); ImGui::SameLine();
        rgbChanged |= ImGui::DragFloat((std::string("##G") + std::to_string(i)).c_str(), &g, 0.001f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::Text("B"); ImGui::SameLine();
        rgbChanged |= ImGui::DragFloat((std::string("##B") + std::to_string(i)).c_str(), &b, 0.001f, 0.0f, 1.0f);
        ImGui::SameLine();
        ImGui::Text("A"); ImGui::SameLine();
        rgbChanged |= ImGui::DragFloat((std::string("##A") + std::to_string(i)).c_str(), &a, 0.001f, 0.0f, 1.0f);
        ImGui::SameLine();

        // Value control
        float hv, hs, hh;
        Utills::RGBtoHSV(colorFloat[0], colorFloat[1], colorFloat[2], hh, hs, hv);
        ImGui::Text("V"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##V") + std::to_string(i)).c_str(), &hv, 0.001f, 0.0f, 1.0f)) {
            float nr, ng, nb;
            Utills::HSVtoRGB(hh, hs, hv, nr, ng, nb);
            __int32 newColor = Utills::Float4ToARGB(nr, ng, nb, colorFloat[3]);
            g_selectedIndexMap[wheelKey] = i;
            PalEdit::ChangeColor(i, newColor);
            currentChar.Character_Colors[i] = newColor;
            PalEdit::Read_Character();
        }
        ImGui::PopItemWidth();

        // Apply RGB changes if any
        if (rgbChanged) {
            g_selectedIndexMap[wheelKey] = i;
            colorValue = Utills::Float4ToARGB(r, g, b, a);
            PalEdit::ChangeColor(i, colorValue);
            PalEdit::Read_Character();
        }

        // Hex input
        {
            unsigned int a_byte = (colorValue >> 24) & 0xFF;
            unsigned int r_byte = (colorValue >> 16) & 0xFF;
            unsigned int g_byte = (colorValue >> 8) & 0xFF;
            unsigned int b_byte = (colorValue) & 0xFF;

            char hexBuf[9];
            sprintf_s(hexBuf, sizeof(hexBuf), "%02X%02X%02X%02X", a_byte, r_byte, g_byte, b_byte);

            ImGui::Text("Hex"); ImGui::SameLine(); ImGui::Text("#"); ImGui::SameLine();
            std::string hexId = std::string("##hex_") + std::to_string(i);

            if (ImGui::InputText(hexId.c_str(), hexBuf, sizeof(hexBuf),
                ImGuiInputTextFlags_CharsHexadecimal |
                ImGuiInputTextFlags_CharsUppercase |
                ImGuiInputTextFlags_EnterReturnsTrue) ||
                ImGui::IsItemDeactivatedAfterEdit()) {

                int len = (int)strlen(hexBuf);
                if (len == 8) {
                    unsigned int v = 0;
                    sscanf_s(hexBuf, "%8X", &v);
                    a_byte = (v >> 24) & 0xFF;
                    r_byte = (v >> 16) & 0xFF;
                    g_byte = (v >> 8) & 0xFF;
                    b_byte = v & 0xFF;
                }
                else if (len == 6) {
                    unsigned int v = 0;
                    sscanf_s(hexBuf, "%6X", &v);
                    r_byte = (v >> 16) & 0xFF;
                    g_byte = (v >> 8) & 0xFF;
                    b_byte = v & 0xFF;
                }

                __int32 newColor = (static_cast<__int32>(a_byte) << 24) |
                    (static_cast<__int32>(r_byte) << 16) |
                    (static_cast<__int32>(g_byte) << 8) |
                    (static_cast<__int32>(b_byte));

                g_selectedIndexMap[wheelKey] = i;
                colorValue = newColor;
                PalEdit::ChangeColor(i, newColor);
                currentChar.Character_Colors[i] = newColor;
                PalEdit::Read_Character();
            }
        }

        // Screen color picker button
        ImGui::SameLine();
        std::string pickId = std::string("Pick##picker_") + std::to_string(i);
        bool isPicking = (g_pickingIndexMap.find(wheelKey) != g_pickingIndexMap.end() &&
            g_pickingIndexMap[wheelKey] == i);

        if (isPicking) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.6f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.6f, 0.9f));
        }

        if (ImGui::Button(pickId.c_str())) {
            // Начинаем процесс пикинга
            g_pickingIndexMap[wheelKey] = i;
            g_pickingPhaseMap[wheelKey] = 1; // Фаза 1: ждем отпускания кнопки
        }

        if (isPicking) {
            ImGui::PopStyleColor(2);
        }

        ImGui::PopID();
        ImGui::Separator();
    }
    ImGui::EndChild();

    // Splitter between Editors and Wheel
    ImGui::SameLine();
    std::string splitterID = std::string("splitter_") + wheelKey;
    ImVec2 splitterPos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(splitterID.c_str(), ImVec2(splitterW, detailsH));

    // Draw splitter
    ImDrawList* local_draw = ImGui::GetWindowDrawList();
    local_draw->AddRectFilled(splitterPos, ImVec2(splitterPos.x + splitterW, splitterPos.y + detailsH), IM_COL32(90, 90, 90, 180));
    local_draw->AddRectFilled(ImVec2(splitterPos.x + 2, splitterPos.y + detailsH * 0.25f),
        ImVec2(splitterPos.x + splitterW - 2, splitterPos.y + detailsH * 0.75f),
        IM_COL32(140, 140, 140, 200));

    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    if (ImGui::IsItemActive()) {
        float mouseDx = ImGui::GetIO().MouseDelta.x;
        wheelWidth -= mouseDx;

        const float minEditors = 120.0f;
        const float minWheel = 160.0f;

        wheelWidth = std::max(minWheel, std::min(wheelWidth, totalW - leftW - minEditors - splitterW));
        editorsWidth = totalW - leftW - wheelWidth - 2.0f * splitterW;
        ratio = wheelWidth / totalW;
        g_wheelRatioMap[wheelKey] = ratio;
    }

    ImGui::SameLine();

    // Wheel canvas
    ImGui::BeginChild("WheelCanvas", ImVec2(wheelWidth, detailsH), false);

    // Обрабатываем пикинг цвета (если активен)
#ifdef _WIN32
    ProcessColorPicker(currentChar, wheelKey);
#endif

    // Draw color wheel
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasAvail = ImGui::GetContentRegionAvail();

    float canvasSize = std::max(64.0f, std::min(canvasAvail.x, detailsH) - 24.0f);
    ImVec2 canvasCenter = ImVec2(canvasPos.x + wheelWidth * 0.5f, canvasPos.y + detailsH * 0.5f);
    float outerR = std::min(canvasSize * 0.45f, wheelWidth * 0.45f);
    float innerR = outerR * 0.20f;

    // Get selected color
    int selected = group.startIndex;
    auto it = g_selectedIndexMap.find(wheelKey);
    if (it != g_selectedIndexMap.end() &&
        it->second >= group.startIndex &&
        it->second < group.startIndex + group.count) {
        selected = it->second;
    }

    // Get selected V for wheel brightness
    float selV = 1.0f;
    if (selected >= 0 && selected < (int)currentChar.Character_Colors.size()) {
        float self[4];
        Utills::ARGBToFloat4(currentChar.Character_Colors[selected], self);
        float h, s, v;
        Utills::RGBtoHSV(self[0], self[1], self[2], h, s, v);
        selV = v;
    }

    // Wheel interaction area
    ImGui::InvisibleButton((std::string("wheel_interact_") + wheelKey).c_str(), ImVec2(wheelWidth, detailsH));
    bool wheelHovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();

    // Draw hue-saturation disk
    const int segments = 128;
    for (int si = 0; si < segments; ++si) {
        float a0 = ((float)si / (float)segments) * 2.0f * 3.14159265f;
        float a1 = ((float)(si + 1) / (float)segments) * 2.0f * 3.14159265f;

        ImVec2 p0 = ImVec2(canvasCenter.x + outerR * cosf(a0), canvasCenter.y + outerR * sinf(a0));
        ImVec2 p1 = ImVec2(canvasCenter.x + outerR * cosf(a1), canvasCenter.y + outerR * sinf(a1));
        ImVec2 q0 = ImVec2(canvasCenter.x + innerR * cosf(a0), canvasCenter.y + innerR * sinf(a0));
        ImVec2 q1 = ImVec2(canvasCenter.x + innerR * cosf(a1), canvasCenter.y + innerR * sinf(a1));

        float hue = (float)si / (float)segments * 360.0f;
        float rr, gg, bb;
        Utills::HSVtoRGB(hue, 1.0f, selV, rr, gg, bb);

        int col = IM_COL32((int)(rr * 255), (int)(gg * 255), (int)(bb * 255), 255);
        ImVec2 poly[4] = { p0, p1, q1, q0 };
        draw_list->AddConvexPolyFilled(poly, 4, col);
    }

    // Inner center
    draw_list->AddCircleFilled(canvasCenter, innerR, IM_COL32(30, 30, 30, 220), 64);

    // Draw nodes
    int nodeRadius = 8;
    for (int idx = 0; idx < group.count && (group.startIndex + idx) < (int)currentChar.Character_Colors.size(); ++idx) {
        int paletteIndex = group.startIndex + idx;
        float cf[4];
        Utills::ARGBToFloat4(currentChar.Character_Colors[paletteIndex], cf);

        float h, s, v;
        Utills::RGBtoHSV(cf[0], cf[1], cf[2], h, s, v);
        float angle = (h / 360.0f) * 2.0f * 3.14159265f;
        float r = innerR + (outerR - innerR) * s;

        ImVec2 pos = ImVec2(canvasCenter.x + r * cosf(angle), canvasCenter.y + r * sinf(angle));
        int col = IM_COL32((int)(cf[0] * 255), (int)(cf[1] * 255), (int)(cf[2] * 255), (int)(cf[3] * 255));

        // Outline
        draw_list->AddCircle(pos, (float)nodeRadius + 1.0f, IM_COL32(20, 20, 20, 220), 16, 1.5f);
        draw_list->AddCircleFilled(pos, (float)nodeRadius, col, 16);

        // Highlight selected
        if (paletteIndex == selected) {
            draw_list->AddCircle(pos, 10.0f, IM_COL32(255, 255, 255, 200), 16, 2.0f);
        }

        // Handle mouse interactions
        if (wheelHovered && ImGui::IsMouseClicked(0)) {
            ImVec2 mp = io.MousePos;
            float dx = mp.x - pos.x;
            float dy = mp.y - pos.y;

            if ((dx * dx + dy * dy) <= (nodeRadius + 4) * (nodeRadius + 4)) {
                g_selectedIndexMap[wheelKey] = paletteIndex;
                g_draggingIndexMap[wheelKey] = paletteIndex;
            }
        }

        // Handle dragging
        auto dit = g_draggingIndexMap.find(wheelKey);
        if (dit != g_draggingIndexMap.end() && dit->second == paletteIndex && io.MouseDown[0]) {
            ImVec2 mp = io.MousePos;
            float dx = mp.x - canvasCenter.x;
            float dy = mp.y - canvasCenter.y;
            float dist = sqrtf(dx * dx + dy * dy);

            float newSat = 0.0f;
            if (dist > innerR) {
                newSat = (dist - innerR) / (outerR - innerR);
            }
            newSat = std::max(0.0f, std::min(newSat, 1.0f));

            float newHue = atan2f(dy, dx) * (180.0f / 3.14159265f);
            if (newHue < 0.0f) newHue += 360.0f;

            float orig[4];
            Utills::ARGBToFloat4(currentChar.Character_Colors[paletteIndex], orig);
            float oh, os, ov;
            Utills::RGBtoHSV(orig[0], orig[1], orig[2], oh, os, ov);

            float nr, ng, nb;
            Utills::HSVtoRGB(newHue, newSat, ov, nr, ng, nb);

            __int32 newColor = Utills::Float4ToARGB(nr, ng, nb, orig[3]);

            PalEdit::ChangeColor(paletteIndex, newColor);
            currentChar.Character_Colors[paletteIndex] = newColor;
            PalEdit::Read_Character();
        }
    }

    // Clear dragging state on mouse release
    if (!io.MouseDown[0]) {
        g_draggingIndexMap.erase(wheelKey);
    }

    ImGui::EndChild(); // WheelCanvas
    ImGui::EndChild(); // WheelMain

    // Отображаем инструкции для пикера
    bool isPicking = (g_pickingIndexMap.find(wheelKey) != g_pickingIndexMap.end());
    if (isPicking) {
        // Создаем стилизованную панель статуса
        ImGui::Separator();

        // Панель с цветным фоном для лучшей видимости
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.0f, 0.8f));
        ImGui::BeginChild("PickerStatus", ImVec2(0, 40), true);

        // Иконка и текст
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Text("⏺ "); // Символ круга или другая иконка
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("Color Picker Active");

        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::Text("Left-click: Pick | Right-click/Esc: Cancel");
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::End();
}