#include "ColorWheel.h"

#include "pch.h"

#include "Files/GroupJSONFiles.h"
#include "PlayableCharactersManager.h"
#include "Tools/Colors.hpp"


// ============================================
// State Management
// ============================================

std::unordered_map<std::string, ColorWheel::WheelState>& ColorWheel::GetStateMap() {
    static std::unordered_map<std::string, WheelState> s_stateMap;
    return s_stateMap;
}

ColorWheel::WheelState& ColorWheel::GetWheelState(const std::string& wheelKey) {
    auto& stateMap = GetStateMap();
    auto it = stateMap.find(wheelKey);
    if (it == stateMap.end()) {
        stateMap[wheelKey] = WheelState();
        stateMap[wheelKey].selectedIndex = 0;
    }
    return stateMap[wheelKey];
}

// ============================================
// Color Utilities
// ============================================

ImVec4 ColorWheel::ARGBToImVec4(__int32 color) {
    return ImVec4(
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f,
        ((color >> 24) & 0xFF) / 255.0f
    );
}

__int32 ColorWheel::ImVec4ToARGB(const ImVec4& color) {
    return (static_cast<__int32>(color.w * 255.0f) << 24) |
        (static_cast<__int32>(color.x * 255.0f) << 16) |
        (static_cast<__int32>(color.y * 255.0f) << 8) |
        (static_cast<__int32>(color.z * 255.0f));
}

// ============================================
// Splitter Drawing
// ============================================

bool ColorWheel::DrawSplitter(const std::string& id, float width, float height,
    float& valueToChange, float minValue, float maxValue) {
    // Получаем текущую позицию сплиттера
    ImVec2 splitterPos = ImGui::GetCursorScreenPos();

    // Создаем невидимую кнопку для сплиттера
    ImGui::InvisibleButton(id.c_str(), ImVec2(width, height));

    // Визуализация сплиттера
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 bgColor = IM_COL32(90, 90, 90, 180);
    ImU32 handleColor = IM_COL32(140, 140, 140, 200);

    // Рисуем фон сплиттера
    drawList->AddRectFilled(splitterPos,
        ImVec2(splitterPos.x + width, splitterPos.y + height),
        bgColor);

    // Рисуем ручку сплиттера (вертикальная полоса)
    drawList->AddRectFilled(
        ImVec2(splitterPos.x + 2, splitterPos.y + height * 0.25f),
        ImVec2(splitterPos.x + width - 2, splitterPos.y + height * 0.75f),
        handleColor
    );

    // Обработка взаимодействия
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    bool changed = false;

    // Изменяем курсор при наведении
    if (hovered || active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    // Обработка перетаскивания
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        // Получаем абсолютное положение мыши
        float mousePosX = ImGui::GetMousePos().x;

        // Вычисляем новое значение на основе позиции мыши
        // Привязываем к левому краю родительского окна
        ImVec2 windowPos = ImGui::GetWindowPos();
        float relativeMouseX = mousePosX - windowPos.x;

        // Корректируем с учетом смещения сплиттера
        // Значение должно быть ограничено minValue и maxValue
        float newValue = std::clamp(relativeMouseX - width / 2.0f, minValue, maxValue);

        // Обновляем значение только если оно изменилось
        if (newValue != valueToChange) {
            valueToChange = newValue;
            changed = true;
        }
    }
    // Альтернативный вариант: использование MouseDelta (если предыдущий не работает)
    else if (active) {
        float dx = ImGui::GetIO().MouseDelta.x;
        if (dx != 0.0f) {
            valueToChange += dx;
            valueToChange = std::clamp(valueToChange, minValue, maxValue);
            changed = true;
        }
    }

    return changed;
}

// ============================================
// Sub-component Drawing
// ============================================

void ColorWheel::DrawColorEditors(const ColorGroup& group,
    const std::string& wheelKey, float width, float height) {
    PlayableCharactersManager& charMgr = PlayableCharactersManager::instance();
    auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
    ImGui::BeginChild("Editors", ImVec2(width, height), false);

    WheelState& state = GetWheelState(wheelKey);

    for (int i = group.startIndex; i < group.startIndex + group.count &&
        i < static_cast<int>(Curent_Char.Character_Colors.size()); ++i) {

        ImU32& colorValue = Curent_Char.Character_Colors[i];
        ImVec4 color = ARGBToImVec4(colorValue);

        ImGui::PushID(i);
        ImGui::Text("Palette Index: %d", i);

        // Large color editor
        if (ImGui::ColorEdit4(("ColorLarge##" + std::to_string(i)).c_str(),
            (float*)&color,
            ImGuiColorEditFlags_AlphaBar |
            ImGuiColorEditFlags_NoInputs |
            ImGuiColorEditFlags_NoLabel)) {
            colorValue = ImVec4ToARGB(color);
            state.selectedIndex = i;
            charMgr.ChangePaletteColor(i, colorValue);
        }

        // Numeric RGB inputs
        ImGui::PushItemWidth(80.0f);

        float r = color.x, g = color.y, b = color.z, a = color.w;

        // R Component
        ImGui::Text("R"); ImGui::SameLine();
        if (ImGui::DragFloat(("##R" + std::to_string(i)).c_str(), &r, 0.001f, 0.0f, 1.0f)) {
            state.selectedIndex = i;
        }

        // G Component
        ImGui::SameLine(); ImGui::Text("G"); ImGui::SameLine();
        if (ImGui::DragFloat(("##G" + std::to_string(i)).c_str(), &g, 0.001f, 0.0f, 1.0f)) {
            state.selectedIndex = i;
        }

        // B Component
        ImGui::SameLine(); ImGui::Text("B"); ImGui::SameLine();
        if (ImGui::DragFloat(("##B" + std::to_string(i)).c_str(), &b, 0.001f, 0.0f, 1.0f)) {
            state.selectedIndex = i;
        }

        // Alpha Component
        ImGui::SameLine(); ImGui::Text("A"); ImGui::SameLine();
        if (ImGui::DragFloat(("##A" + std::to_string(i)).c_str(), &a, 0.001f, 0.0f, 1.0f)) {
            state.selectedIndex = i;
        }

        // Value (Brightness) control using HSV
        ImGui::SameLine(); ImGui::Text("V"); ImGui::SameLine();

        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);

        if (ImGui::DragFloat(("##V" + std::to_string(i)).c_str(), &v, 0.001f, 0.0f, 1.0f)) {
            // Convert back to RGB with new value
            ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
            color = ImVec4(r, g, b, a);
            colorValue = ImVec4ToARGB(color);
            state.selectedIndex = i;
            charMgr.ChangePaletteColor(i, colorValue);
        }

        ImGui::PopItemWidth();

        // Apply changes from RGB inputs
        if (r != color.x || g != color.y || b != color.z || a != color.w) {
            color = ImVec4(r, g, b, a);
            colorValue = ImVec4ToARGB(color);
            charMgr.ChangePaletteColor(i, colorValue);
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    ImGui::EndChild();
}

void ColorWheel::DrawColorWheel(const ColorGroup& group,
    const std::string& wheelKey, float width, float height) {
    PlayableCharactersManager& charMgr = PlayableCharactersManager::instance();
    auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
    ImGui::BeginChild("WheelCanvas", ImVec2(width, height), false);

    WheelState& state = GetWheelState(wheelKey);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasAvail = ImGui::GetContentRegionAvail();

    // Calculate wheel dimensions
    float canvasSize = std::min(canvasAvail.x, height) - 24.0f;
    canvasSize = std::max(canvasSize, 64.0f);

    ImVec2 canvasCenter = ImVec2(canvasPos.x + width * 0.5f, canvasPos.y + height * 0.5f);
    float outerRadius = std::min(canvasSize * 0.45f, width * 0.45f);
    float innerRadius = outerRadius * 0.2f;

    // Validate selected index
    if (state.selectedIndex < group.startIndex ||
        state.selectedIndex >= group.startIndex + group.count) {
        state.selectedIndex = group.startIndex;
    }

    // Get brightness from selected color for wheel background
    float wheelBrightness = 1.0f;
    //if (state.selectedIndex >= 0 &&
    //    state.selectedIndex < static_cast<int>(Curent_Char.Character_Colors.size())) {
    //    ImVec4 selectedColor = ARGBToImVec4(Curent_Char.Character_Colors[state.selectedIndex]);
    //    float h, s, v;
    //    ImGui::ColorConvertRGBtoHSV(selectedColor.x, selectedColor.y, selectedColor.z, h, s, v);
    //    wheelBrightness = v;
    //}

    // Interaction area
    ImGui::InvisibleButton(("wheel_interact_" + wheelKey).c_str(), ImVec2(width, height));
    bool wheelHovered = ImGui::IsItemHovered();
    bool wheelActive = ImGui::IsItemActive();
    ImGuiIO& io = ImGui::GetIO();

    // Draw hue-saturation wheel
    const int segments = 128;
    for (int i = 0; i < segments; ++i) {
        float angle0 = (static_cast<float>(i) / segments) * 2.0f * IM_PI;
        float angle1 = (static_cast<float>(i + 1) / segments) * 2.0f * IM_PI;

        ImVec2 outer0 = ImVec2(canvasCenter.x + outerRadius * cosf(angle0),
            canvasCenter.y + outerRadius * sinf(angle0));
        ImVec2 outer1 = ImVec2(canvasCenter.x + outerRadius * cosf(angle1),
            canvasCenter.y + outerRadius * sinf(angle1));
        ImVec2 inner0 = ImVec2(canvasCenter.x + innerRadius * cosf(angle0),
            canvasCenter.y + innerRadius * sinf(angle0));
        ImVec2 inner1 = ImVec2(canvasCenter.x + innerRadius * cosf(angle1),
            canvasCenter.y + innerRadius * sinf(angle1));

        float hue = static_cast<float>(i) / segments * 360.0f;
        ImVec4 segmentColor;
        ImGui::ColorConvertHSVtoRGB(hue / 360.0f, 1.0f, wheelBrightness,
            segmentColor.x, segmentColor.y, segmentColor.z);
        segmentColor.w = 1.0f;

        ImVec2 polygon[4] = { outer0, outer1, inner1, inner0 };
        drawList->AddConvexPolyFilled(polygon, 4, ImGui::ColorConvertFloat4ToU32(segmentColor));
    }

    // Draw center
    drawList->AddCircleFilled(canvasCenter, innerRadius, IM_COL32(30, 30, 30, 220), 64);

    // Draw color nodes
    const int nodeRadius = 8;
    for (int idx = 0; idx < group.count; ++idx) {
        int paletteIndex = group.startIndex + idx;
        if (paletteIndex >= static_cast<int>(Curent_Char.Character_Colors.size())) break;

        ImVec4 color = ARGBToImVec4(Curent_Char.Character_Colors[paletteIndex]);
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);

        // Convert hue from 0-1 to 0-360 for positioning
        h *= 360.0f;

        // Calculate position on wheel
        float angle = h / 360.0f * 2.0f * IM_PI;
        float radius = innerRadius + (outerRadius - innerRadius) * s;
        ImVec2 position = ImVec2(canvasCenter.x + radius * cosf(angle),
            canvasCenter.y + radius * sinf(angle));

        // Draw node with outline
        drawList->AddCircle(position, nodeRadius + 1.0f, IM_COL32(20, 20, 20, 220), 16, 1.5f);
        drawList->AddCircleFilled(position, nodeRadius, ImGui::ColorConvertFloat4ToU32(color), 16);

        // Highlight selected node
        if (paletteIndex == state.selectedIndex) {
            drawList->AddCircle(position, nodeRadius + 2.0f, IM_COL32(255, 255, 255, 200), 16, 2.0f);
        }

        // Handle node interactions
        if (wheelHovered && ImGui::IsMouseClicked(0)) {
            ImVec2 mousePos = io.MousePos;
            float dx = mousePos.x - position.x;
            float dy = mousePos.y - position.y;
            float distanceSquared = dx * dx + dy * dy;

            if (distanceSquared <= (nodeRadius + 4) * (nodeRadius + 4)) {
                state.selectedIndex = paletteIndex;
                state.draggingIndex = paletteIndex;
            }
        }

        // Handle dragging
        if (state.draggingIndex == paletteIndex && io.MouseDown[0]) {
            ImVec2 mousePos = io.MousePos;
            float dx = mousePos.x - canvasCenter.x;
            float dy = mousePos.y - canvasCenter.y;

            // Calculate new hue and saturation
            float distance = sqrtf(dx * dx + dy * dy);
            float newSaturation = 0.0f;

            if (distance > innerRadius) {
                newSaturation = (distance - innerRadius) / (outerRadius - innerRadius);
                newSaturation = std::clamp(newSaturation, 0.0f, 1.0f);
            }

            float newHue = atan2f(dy, dx) * (180.0f / IM_PI);
            if (newHue < 0.0f) newHue += 360.0f;

            // Convert hue to 0-1 range for ImGui
            newHue /= 360.0f;

            // Preserve original value and alpha
            ImVec4 originalColor = ARGBToImVec4(Curent_Char.Character_Colors[paletteIndex]);
            float originalH, originalS, originalV;
            ImGui::ColorConvertRGBtoHSV(originalColor.x, originalColor.y, originalColor.z,
                originalH, originalS, originalV);

            // Create new color with updated hue/saturation
            ImVec4 newColor;
            ImGui::ColorConvertHSVtoRGB(newHue, newSaturation, originalV,
                newColor.x, newColor.y, newColor.z);
            newColor.w = originalColor.w;

            ImU32 newARGB = ImVec4ToARGB(newColor);
            charMgr.ChangePaletteColor(paletteIndex, newARGB);
        }

        // Stop dragging on mouse release
        if (!io.MouseDown[0]) {
            state.draggingIndex = -1;
        }
    }

    ImGui::EndChild();
}

// ============================================
// Main Drawing Function
// ============================================

void ColorWheel::Draw(const ColorGroup& group, bool& open) {
    auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();
    std::string wheelKey = std::string(Curent_Char.Char_Name) + "|" + group.groupName;
    std::string windowTitle = "Color Wheel - " + wheelKey;

    // Window setup
    ImGui::SetNextWindowSize(ImVec2(800, 480), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 240), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin(windowTitle.c_str(), &open)) {
        ImGui::End();
        return;
    }

    // Get available space
    ImVec2 availableSpace = ImGui::GetContentRegionAvail();
    float childHeight = availableSpace.y > 0.0f ? availableSpace.y : 400.0f;

    ImGui::BeginChild("WheelMain", ImVec2(0, childHeight), false);

    // Calculate layout
    ImVec2 totalAvailable = ImGui::GetContentRegionAvail();
    float totalWidth = totalAvailable.x > 0.0f ? totalAvailable.x : 800.0f;

    WheelState& state = GetWheelState(wheelKey);

    // Clamp persisted value
    state.wheelRatio = std::clamp(state.wheelRatio, 0.2f, 0.8f);

    const float splitterWidth = 6.0f;
    const float minEditorsWidth = 120.0f;
    const float minWheelWidth = 160.0f;

    // Calculate widths based on ratio
    float wheelWidth = std::max(minWheelWidth, totalWidth * state.wheelRatio);
    float editorsWidth = totalWidth - wheelWidth - splitterWidth;

    // Adjust if editors too narrow
    if (editorsWidth < minEditorsWidth) {
        editorsWidth = minEditorsWidth;
        wheelWidth = totalWidth - editorsWidth - splitterWidth;
        wheelWidth = std::max(wheelWidth, minWheelWidth);
        state.wheelRatio = wheelWidth / totalWidth;
    }

    // Draw color editors
    DrawColorEditors(group, wheelKey, editorsWidth, childHeight);

    // Splitter (between editors and wheel)
    ImGui::SameLine();

    // Рассчитываем позицию сплиттера в абсолютных координатах
    // Это нужно для правильного расчета новой ширины при перетаскивании
    static float splitterPosX = 0;

    // Вычисляем диапазон, в котором может двигаться сплиттер
    float minSplitterPos = minEditorsWidth;
    float maxSplitterPos = totalWidth - minWheelWidth - splitterWidth;

    // Текущая позиция сплиттера (ширина редакторов)
    float currentSplitterPos = editorsWidth;

    // Draw splitter
    bool splitterChanged = DrawSplitter("splitter_" + wheelKey, splitterWidth, childHeight,
        currentSplitterPos, minSplitterPos, maxSplitterPos);

    // Если сплиттер переместили, обновляем ширины
    if (splitterChanged) {
        editorsWidth = currentSplitterPos;
        wheelWidth = totalWidth - editorsWidth - splitterWidth;
        state.wheelRatio = wheelWidth / totalWidth;
    }

    // Draw color wheel
    ImGui::SameLine();
    DrawColorWheel(group, wheelKey, wheelWidth, childHeight);

    ImGui::EndChild(); // WheelMain
    ImGui::End();
}