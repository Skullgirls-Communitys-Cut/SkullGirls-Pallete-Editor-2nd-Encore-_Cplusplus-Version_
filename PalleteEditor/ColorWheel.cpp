#include "ColorWheel.h"
#include "PalleteEditor.h"
#include "ImGui/imgui.h"
#include <string>
#include <unordered_map>
#include <cmath>

// per-wheel selected index (persisted by character|group key)
static std::unordered_map<std::string, int> g_selectedIndexMap;
// per-wheel layout ratio (fraction of Details width allocated to the wheel)
static std::unordered_map<std::string, float> g_wheelRatioMap;
// per-wheel left column width (swatches)
static std::unordered_map<std::string, float> g_leftWidthMap;
// dragging state: which palette index is currently being dragged per wheel
static std::unordered_map<std::string, int> g_draggingIndexMap;

// Convert RGB (0..1) to HSV (h in degrees 0..360, s,v 0..1)
static void RGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
{
    float max = std::fmax(r, std::fmax(g, b));
    float min = std::fmin(r, std::fmin(g, b));
    out_v = max;
    float d = max - min;
    out_s = max == 0.0f ? 0.0f : d / max;
    if (d == 0.0f) { out_h = 0.0f; return; }
    if (max == r) out_h = 60.0f * (fmod(((g - b) / d), 6.0f));
    else if (max == g) out_h = 60.0f * (((b - r) / d) + 2.0f);
    else out_h = 60.0f * (((r - g) / d) + 4.0f);
    if (out_h < 0.0f) out_h += 360.0f;
}

// Convert HSV (h in degrees 0..360, s,v 0..1) to RGB (0..1)
static void HSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
{
    float C = v * s;
    float X = C * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - C;
    float r=0,g=0,b=0;
    if (h < 60.0f) { r = C; g = X; b = 0; }
    else if (h < 120.0f) { r = X; g = C; b = 0; }
    else if (h < 180.0f) { r = 0; g = C; b = X; }
    else if (h < 240.0f) { r = 0; g = X; b = C; }
    else if (h < 300.0f) { r = X; g = 0; b = C; }
    else { r = C; g = 0; b = X; }
    out_r = r + m; out_g = g + m; out_b = b + m;
}

// Convert ARGB int to float[4]
static void ARGBToFloat4(__int32 cVal, float out[4])
{
    out[0] = ((cVal >> 16) & 0xFF) / 255.0f;
    out[1] = ((cVal >> 8) & 0xFF) / 255.0f;
    out[2] = (cVal & 0xFF) / 255.0f;
    out[3] = ((cVal >> 24) & 0xFF) / 255.0f;
}

// Compose ARGB int from floats (r,g,b,a in 0..1)
static inline __int32 Float4ToARGB(float r, float g, float b, float a)
{
    return (static_cast<__int32>(a * 255.0f) << 24) |
           (static_cast<__int32>(r * 255.0f) << 16) |
           (static_cast<__int32>(g * 255.0f) << 8) |
           (static_cast<__int32>(b * 255.0f));
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
        float sw[4]; ARGBToFloat4(cVal, sw);
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
            colorValue = Float4ToARGB(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);

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
        RGBtoHSV(colorFloat[0], colorFloat[1], colorFloat[2], hh, hs, hv);
        ImGui::Text("V"); ImGui::SameLine();
        if (ImGui::DragFloat((std::string("##V") + std::to_string(i)).c_str(), &hv, 0.001f, 0.0f, 1.0f)) {
            float nr,ng,nb; HSVtoRGB(hh, hs, hv, nr, ng, nb);
            colorFloat[0] = nr; colorFloat[1] = ng; colorFloat[2] = nb;
            // compose and apply immediately
            __int32 newColor = Float4ToARGB(nr, ng, nb, colorFloat[3]);
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
            colorValue = Float4ToARGB(colorFloat[0], colorFloat[1], colorFloat[2], colorFloat[3]);
            PalEdit::ChangeColor(i, colorValue);
            PalEdit::Read_Character();
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
        float self[4]; ARGBToFloat4(currentChar.Character_Colors[selected], self);
        float h,s,v; RGBtoHSV(self[0], self[1], self[2], h, s, v);
        selV = v;
    }

    // Reserve interaction area for the wheel (entire child) so we can detect clicks/drags
    ImGuiIO& io = ImGui::GetIO();
    ImGui::InvisibleButton((std::string("wheel_interact_") + wheelKey).c_str(), ImVec2(wheelWidth, detailsH));
    bool wheelHovered = ImGui::IsItemHovered();
    bool wheelActive = ImGui::IsItemActive();

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
        float rr,gg,bb; HSVtoRGB(hue, 1.0f, selV, rr, gg, bb);
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
        float cf[4]; ARGBToFloat4(currentChar.Character_Colors[paletteIndex], cf);
        float h,s,v; RGBtoHSV(cf[0], cf[1], cf[2], h, s, v);
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
            float orig[4]; ARGBToFloat4(currentChar.Character_Colors[paletteIndex], orig);
            float oh,os,ov; RGBtoHSV(orig[0], orig[1], orig[2], oh, os, ov);
            float nr,ng,nb; HSVtoRGB(newHue, newSat, ov, nr, ng, nb);
            __int32 newColor = Float4ToARGB(nr, ng, nb, orig[3]);
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
