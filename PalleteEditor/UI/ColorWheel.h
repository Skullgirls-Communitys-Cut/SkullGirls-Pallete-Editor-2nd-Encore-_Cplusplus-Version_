#pragma once
#include "Files/GroupJSONFiles.h"
#include "pch.h"
class ColorWheel {
private:
    struct WheelState {
        int selectedIndex = 0;
        float wheelRatio = 0.45f;
        float leftWidth = 220.0f;
        int draggingIndex = -1;
    };
public:
    // Draw the color-wheel/detail window for a specific character and color group.
    // `open` is a reference to the boolean which controls the window visibility.
    static void Draw(const ColorGroup& group, bool& open);

    // State storage
    static std::unordered_map<std::string, WheelState>& GetStateMap();
private:
    // Internal state management


    static WheelState& GetWheelState(const std::string& wheelKey);

    static void DrawColorEditors(const ColorGroup& group,
        const std::string& wheelKey, float width, float height);
    static void DrawColorWheel(const ColorGroup& group,
        const std::string& wheelKey, float width, float height);

    // Splitter drawing
    static bool DrawSplitter(const std::string& id, float width, float height,
        float& valueToChange, float minValue, float maxValue);

    // Color utilities using ImGui
    static ImVec4 ARGBToImVec4(__int32 color);
    static __int32 ImVec4ToARGB(const ImVec4& color);


};