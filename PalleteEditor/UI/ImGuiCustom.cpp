#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#include "ImGuiCustom.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

#include "Eyedropper/Eyedropper.h"

ImVec4 ImGuiCustom::saved_palette[32];

using namespace ImGui;

static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
        return;

    // When S == 0, H is undefined.
    // When H == 1 it wraps around to 0.
    if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
        *H = g.ColorEditSavedHue;

    // When V == 0, S is undefined.
    if (*V == 0.0f)
        *S = g.ColorEditSavedSat;
}

static void ColorEditRestoreH(const float* col, float* H)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
        return;
    *H = g.ColorEditSavedHue;
}

static void RenderArrowsForHorizontalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_h, float alpha)
{
    ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x, pos.y + half_sz.y + 1), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Down, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x, pos.y + half_sz.y), half_sz, ImGuiDir_Down, IM_COL32(255, 255, 255, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x, pos.y + bar_h - half_sz.y - 1), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Up, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x, pos.y + bar_h - half_sz.y), half_sz, ImGuiDir_Up, IM_COL32(255, 255, 255, alpha8));
}

static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w, float alpha)
{
    ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x + 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
    ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
}

static bool SliderScalarGradient(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const ImU32 color_start, const ImU32 color_end, const char* format, ImGuiSliderFlags flags = 0)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Slider turns it into an input box
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp Ctrl+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);
    // --------------RGB-Sliders edit block start-------------
    // RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);
    RenderColorRectWithAlphaCheckerboard(window->DrawList, frame_bb.Min, frame_bb.Max, 0, frame_bb.GetHeight() / 2.0f, ImVec2(0.0f, 0.0f));
    window->DrawList->AddRectFilledMultiColor(frame_bb.Min, frame_bb.Max, color_start, color_end, color_end, color_start);
    //RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);
    // --------------RGB-Sliders edit block end -------------

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // --------------RGB-Sliders edit block start-------------
    // Render grab
    //if (grab_bb.Max.x > grab_bb.Min.x)
    //   window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    float bars_triangles_half_sz = IM_FLOOR(frame_bb.GetHeight() * 0.20f);
    RenderArrowsForHorizontalBar(window->DrawList, ImVec2(grab_bb.GetCenter().x, frame_bb.Min.y), ImVec2(bars_triangles_half_sz, bars_triangles_half_sz + 1), frame_bb.GetHeight(), style.Alpha);
    // --------------RGB-Sliders edit block end -------------

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

static bool SliderScalarGradientHue(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags = 0)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Slider turns it into an input box
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp Ctrl+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(frame_bb, id);

    // Отрисовка радужного градиента для Hue
    const ImU32 hue_colors[] = {
        IM_COL32(255, 0, 0, 255),     // Red
        IM_COL32(255, 255, 0, 255),   // Yellow
        IM_COL32(0, 255, 0, 255),     // Green
        IM_COL32(0, 255, 255, 255),   // Cyan
        IM_COL32(0, 0, 255, 255),     // Blue
        IM_COL32(255, 0, 255, 255),   // Magenta
        IM_COL32(255, 0, 0, 255)      // Red (замыкаем)
    };

    // Разбиваем прямоугольник на 6 сегментов для радуги
    const int num_segments = 6;
    const float segment_width = frame_bb.GetWidth() / num_segments;

    for (int i = 0; i < num_segments; i++)
    {
        ImVec2 segment_min = ImVec2(frame_bb.Min.x + i * segment_width, frame_bb.Min.y);
        ImVec2 segment_max = ImVec2(segment_min.x + segment_width, frame_bb.Max.y);

        // Интерполируем цвет между двумя соседними цветами радуги
        window->DrawList->AddRectFilledMultiColor(
            segment_min, segment_max,
            hue_colors[i], hue_colors[i + 1], hue_colors[i + 1], hue_colors[i]
        );
    }

    // Слайдер поведение
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Рендерим стрелки для слайдера Hue (горизонтальные)
    float bars_triangles_half_sz = IM_FLOOR(frame_bb.GetHeight() * 0.20f);
    RenderArrowsForHorizontalBar(window->DrawList, ImVec2(grab_bb.GetCenter().x, frame_bb.Min.y), ImVec2(bars_triangles_half_sz, bars_triangles_half_sz + 1), frame_bb.GetHeight(), style.Alpha);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool ImGuiCustom::ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float square_sz = GetFrameHeight();
    const char* label_display_end = FindRenderedTextEnd(label);
    float w_full = CalcItemWidth();
    g.NextItemData.ClearFlags();

    // Static palette initialization
    static bool saved_palette_init = true;
    if (saved_palette_init)
    {
        for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
        {
            ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
            saved_palette[n].w = 1.0f; // Alpha
        }
        saved_palette_init = false;
    }

    BeginGroup();
    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();

    // If we're not showing any slider there's no point in doing any HSV conversions
    const ImGuiColorEditFlags flags_untouched = flags;
    if (flags & ImGuiColorEditFlags_NoInputs)
        flags = (flags & (~ImGuiColorEditFlags_DisplayMask_)) | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoOptions;

    // Context menu: display and modify options (before defaults are applied)
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorEditOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags_DisplayMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DisplayMask_);
    if (!(flags & ImGuiColorEditFlags_DataTypeMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DataTypeMask_);
    if (!(flags & ImGuiColorEditFlags_PickerMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_);
    if (!(flags & ImGuiColorEditFlags_InputMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_InputMask_);
    flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_));
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_DisplayMask_)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));   // Check that only 1 is selected

    const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
    const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
    const int components = alpha ? 4 : 3;
    const float w_button = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
    const float w_inputs = ImMax(w_full - w_button, 1.0f);
    w_full = w_inputs + w_button;

    // Convert to the formats we need
    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    if ((flags & ImGuiColorEditFlags_InputHSV) && (flags & ImGuiColorEditFlags_DisplayRGB))
        ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
    else if ((flags & ImGuiColorEditFlags_InputRGB) && (flags & ImGuiColorEditFlags_DisplayHSV))
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
        ColorEditRestoreHS(col, &f[0], &f[1], &f[2]);
    }
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    bool value_changed = false;
    bool value_changed_as_float = false;
    static ImVec4 backup_color;

    const ImVec2 pos = window->DC.CursorPos;
    const float inputs_offset_x = (style.ColorButtonPosition == ImGuiDir_Left) ? w_button : 0.0f;
    window->DC.CursorPos.x = pos.x + inputs_offset_x;

    if ((flags & (ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB/HSV 0..255 Sliders
        const float w_items = w_inputs - style.ItemInnerSpacing.x * (components - 1);

        const bool hide_prefix = (IM_TRUNC(w_items / components) <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
        static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
        static const char* fmt_table_int[3][4] =
        {
            {   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
            { "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
            { "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
        };
        static const char* fmt_table_float[3][4] =
        {
            {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
            { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
            { "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
        };
        const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_DisplayHSV) ? 2 : 1;

        float prev_split = 0.0f;
        for (int n = 0; n < components; n++)
        {
            //if (n > 0)
            //    SameLine(0, style.ItemInnerSpacing.x);
            //SetNextItemWidth(w_items);

            // FIXME: When ImGuiColorEditFlags_HDR flag is passed HS values snap in weird ways when SV values go below 0.
            if (flags & ImGuiColorEditFlags_Float)
            {
                // --------------RGB-Sliders edit block start-------------
                if (flags & ImGuiColorEditFlags_DisplayRGB)
                {
                    float zero = 0.0f;
                    float one = 1.0f;
                    float g0[4];
                    float g1[4];
                    for (int i = 0; i < 4; i++)
                    {
                        if (i != n)
                        {
                            g1[i] = g0[i] = f[i];
                        }
                        else
                        {
                            g0[i] = 0.0f;
                            g1[i] = 1.0f;
                        }
                    }
                    if (n < 3) {
                        g0[3] = g1[3] = 1.0;
                    }
                    ImU32 color_start = GetColorU32(ImVec4(g0[0], g0[1], g0[2], g0[3]));
                    ImU32 color_end = GetColorU32(ImVec4(g1[0], g1[1], g1[2], g1[3]));
                    value_changed |= SliderScalarGradient(ids[n], ImGuiDataType_Float, &f[n], &zero, &one, color_start, color_end, fmt_table_float[fmt_idx][n]);
                }
                else
                {
                    value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
                }
                // --------------RGB-Sliders edit block end-------------
                value_changed_as_float |= value_changed;
            }
            else
            {
                // --------------RGB-Sliders for integer mode (0-255)-------------
                if (flags & ImGuiColorEditFlags_DisplayRGB)
                {
                    int zero = 0;
                    int one = 255;
                    uint8_t g0[4];
                    uint8_t g1[4];
                    for (int component_t = 0; component_t < 4; component_t++)
                    {
                        if (component_t != n)
                        {
                            g1[component_t] = g0[component_t] = static_cast<uint8_t>(i[component_t]);
                        }
                        else
                        {
                            g0[component_t] = 0;
                            g1[component_t] = 255;
                        }
                    }
                    if (n < 3) {
                        g0[3] = g1[3] = 255;
                    }

                    ImU32 color_start = IM_COL32(g0[0], g0[1], g0[2], g0[3]);
                    ImU32 color_end = IM_COL32(g1[0], g1[1], g1[2], g1[3]);

                    value_changed |= SliderScalarGradient(ids[n], ImGuiDataType_S32, &i[n], &zero, &one, color_start, color_end, fmt_table_int[fmt_idx][n]);
                }
                else
                {
                    if (n == 0) // Hue - используем специальный радужный слайдер
                    {
                        int zero = 0;
                        int one = 255; // Hue от 0 до 360 градусов
                        value_changed |= SliderScalarGradientHue(ids[n], ImGuiDataType_S32, &i[n], &zero, &one, fmt_table_int[2][n]);
                    }
                    else if (n == 1) // Saturation
                    {
                        int zero = 0;
                        int one = 255;
                        // Вычисляем цвета для градиента насыщенности
                        // При насыщенности 0 - серый цвет (только значение)
                        // При насыщенности 1 - полный цвет
                        float hue = i[0] / 255.0f;
                        float value = i[2] / 255.0f;

                        // Цвет при насыщенности 0
                        float r0, g0, b0;
                        ColorConvertHSVtoRGB(hue, 0.0f, value, r0, g0, b0);

                        // Цвет при насыщенности 1
                        float r1, g1, b1;
                        ColorConvertHSVtoRGB(hue, 1.0f, value, r1, g1, b1);

                        ImU32 color_start = GetColorU32(ImVec4(r0, g0, b0, 1.0f));
                        ImU32 color_end = GetColorU32(ImVec4(r1, g1, b1, 1.0f));

                        value_changed |= SliderScalarGradient(ids[n], ImGuiDataType_S32, &i[n], &zero, &one, color_start, color_end, fmt_table_int[2][n]);
                    }
                    else if (n == 2) // Value (Яркость)
                    {
                        int zero = 0;
                        int one = 255;
                        // Вычисляем цвета для градиента яркости
                        // При значении 0 - черный
                        // При значении 1 - полный цвет
                        float hue = i[0] / 255.0f;
                        float saturation = i[1] / 255.0f;

                        // Цвет при значении 0 (черный)
                        ImU32 color_start = GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

                        // Цвет при значении 1
                        float r, g, b;
                        ColorConvertHSVtoRGB(hue, saturation, 1.0f, r, g, b);
                        ImU32 color_end = GetColorU32(ImVec4(r, g, b, 1.0f));

                        value_changed |= SliderScalarGradient(ids[n], ImGuiDataType_S32, &i[n], &zero, &one, color_start, color_end, fmt_table_int[2][n]);
                    }
                    else if (n == 3) // Alpha
                    {
                        int zero = 0;
                        int one = 255;
                        // Для альфа-канала используем градиент от прозрачного к непрозрачному
                        // Базовый цвет берем из текущих HSV значений
                        float hue = i[0] / 255.0f;
                        float saturation = i[1] / 255.0f;
                        float value = i[2] / 255.0f;

                        float r, g, b;
                        ColorConvertHSVtoRGB(hue, saturation, value, r, g, b);

                        ImU32 color_start = GetColorU32(ImVec4(r, g, b, 0.0f)); // Прозрачный
                        ImU32 color_end = GetColorU32(ImVec4(r, g, b, 1.0f));   // Непрозрачный

                        value_changed |= SliderScalarGradient(ids[n], ImGuiDataType_S32, &i[n], &zero, &one, color_start, color_end, fmt_table_int[2][n]);
                    }
                }
                // --------------RGB-Sliders edit block end-------------
            }
            if (!(flags & ImGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
        }
    }
    else if ((flags & ImGuiColorEditFlags_DisplayHex) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB Hexadecimal Input
        char buf[64];
        if (alpha)
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
        else
            ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
        SetNextItemWidth(w_inputs);
        if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsUppercase))
        {
            value_changed = true;
            char* p = buf;
            while (*p == '#' || ImCharIsBlankA(*p))
                p++;
            i[0] = i[1] = i[2] = 0;
            i[3] = 0xFF; // alpha default to 255 is not parsed by scanf (e.g. inputting #FFFFFF omitting alpha)
            int r;
            if (alpha)
                r = sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
            else
                r = sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
            IM_UNUSED(r); // Fixes C6031: Return value ignored: 'sscanf'.
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
    }

    ImGuiWindow* picker_active_window = NULL;
    if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
    {
        const float button_offset_x = ((flags & ImGuiColorEditFlags_NoInputs) || (style.ColorButtonPosition == ImGuiDir_Left)) ? 0.0f : w_inputs + style.ItemInnerSpacing.x;
        window->DC.CursorPos = ImVec2(pos.x + button_offset_x, pos.y);

        const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);

        // Custom palette button
        bool open_popup = ColorButton("##ColorButton", col_v4, flags);
        if (open_popup)
        {
            backup_color = col_v4;
            OpenPopup("picker");
            SetNextWindowPos(g.LastItemData.Rect.GetBL() + ImVec2(0.0f, style.ItemSpacing.y));
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);

        if (BeginPopup("picker"))
        {
            if (g.CurrentWindow->BeginCount == 1)
            {
                picker_active_window = g.CurrentWindow;
                if (label != label_display_end)
                {
                    TextEx(label, label_display_end);
                    Spacing();
                }
                ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
                ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
                SetNextItemWidth(square_sz * 12.0f);
                value_changed |= ImGuiCustom::ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);

                // Custom palette section
                //SameLine();
                Separator();
                BeginGroup();
                Text("Palette");
                for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
                {
                    PushID(n);
                    if ((n % 8) != 0)
                        SameLine(0.0f, style.ItemSpacing.y);

                    ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                    if (ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                    {
                        col[0] = ImGuiCustom::saved_palette[n].x;
                        col[1] = ImGuiCustom::saved_palette[n].y;
                        col[2] = ImGuiCustom::saved_palette[n].z;
                        // Preserve alpha
                        value_changed = true;
                    }

                    // Allow user to drop colors into each palette entry
                    if (BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                            memcpy((float*)&ImGuiCustom::saved_palette[n], payload->Data, sizeof(float) * 3);
                        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                            memcpy((float*)&ImGuiCustom::saved_palette[n], payload->Data, sizeof(float) * 4);
                        EndDragDropTarget();
                    }

                    PopID();
                }
                EndGroup();
            }
            EndPopup();
        }
    }

    if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
    {
        SameLine(0.0f, style.ItemInnerSpacing.x);
        window->DC.CursorPos.x = pos.x + ((flags & ImGuiColorEditFlags_NoInputs) ? w_button : w_full + style.ItemInnerSpacing.x);
        TextEx(label, label_display_end);
    }

    // Convert back
    if (value_changed && picker_active_window == NULL)
    {
        if (!value_changed_as_float)
            for (int n = 0; n < 4; n++)
                f[n] = i[n] / 255.0f;
        if ((flags & ImGuiColorEditFlags_DisplayHSV) && (flags & ImGuiColorEditFlags_InputRGB))
        {
            g.ColorEditSavedHue = f[0];
            g.ColorEditSavedSat = f[1];
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(f[0], f[1], f[2], 0));
        }
        if ((flags & ImGuiColorEditFlags_DisplayRGB) && (flags & ImGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

        col[0] = f[0];
        col[1] = f[1];
        col[2] = f[2];
        if (alpha)
            col[3] = f[3];
    }

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();
    EndGroup();

    // Drag and Drop Target
    if ((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
    {
        bool accepted_drag_drop = false;
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * 3);
            value_changed = accepted_drag_drop = true;
        }
        if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
        {
            memcpy((float*)col, payload->Data, sizeof(float) * components);
            value_changed = accepted_drag_drop = true;
        }

        // Drag-drop payloads are always RGB
        if (accepted_drag_drop && (flags & ImGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(col[0], col[1], col[2], col[0], col[1], col[2]);
        EndDragDropTarget();
    }

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
        g.LastItemData.ID = g.ActiveId;

    if (value_changed && g.LastItemData.ID != 0)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool ImGuiCustom::ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImDrawList* draw_list = window->DrawList;
    ImGuiStyle& style = g.Style;
    ImGuiIO& io = g.IO;

    const float width = CalcItemWidth();
    const bool is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & ImGuiItemFlags_ReadOnly) != 0;
    g.NextItemData.ClearFlags();

    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();
    BeginGroup();
    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
        flags |= ImGuiColorEditFlags_NoSmallPreview;

    // Context menu: display and store options.
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorPickerOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags_PickerMask_))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_PickerMask_;
    if (!(flags & ImGuiColorEditFlags_InputMask_))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_InputMask_;
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_PickerMask_)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));  // Check that only 1 is selected
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

    // Setup
    int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
    bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
    ImVec2 picker_pos = window->DC.CursorPos;
    float square_sz = GetFrameHeight();
    float sliders_height = (11 * GetFrameHeight()) + ((components)*style.ItemInnerSpacing.y);
    float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
    float sv_picker_size = ImMax(bars_width * 1, width - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
    if (sv_picker_size > width * 0.5f) // Ограничить квадрат 50% ширины
        sv_picker_size = width * 0.5f;
    sv_picker_size = ImMax(sv_picker_size, sliders_height);

    float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
    float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
    float bars_triangles_half_sz = IM_TRUNC(bars_width * 0.20f);

    float backup_initial_col[4];
    memcpy(backup_initial_col, col, components * sizeof(float));

    float wheel_thickness = sv_picker_size * 0.08f;
    float wheel_r_outer = sv_picker_size * 0.50f;
    float wheel_r_inner = wheel_r_outer - wheel_thickness;
    ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width) * 0.5f, picker_pos.y + sv_picker_size * 0.5f);

    // Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
    float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
    ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
    ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
    ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

    float H = col[0], S = col[1], V = col[2];
    float R = col[0], G = col[1], B = col[2];
    if (flags & ImGuiColorEditFlags_InputRGB)
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(R, G, B, H, S, V);
        ColorEditRestoreHS(col, &H, &S, &V);
    }
    else if (flags & ImGuiColorEditFlags_InputHSV)
    {
        ColorConvertHSVtoRGB(H, S, V, R, G, B);
    }

    bool value_changed = false, value_changed_h = false, value_changed_sv = false;

    PushItemFlag(ImGuiItemFlags_NoNav, true);
    if (flags & ImGuiColorEditFlags_PickerHueWheel)
    {
        // Hue wheel + SV triangle logic
        InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
        if (IsItemActive() && !is_readonly)
        {
            ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
            ImVec2 current_off = g.IO.MousePos - wheel_center;
            float initial_dist2 = ImLengthSqr(initial_off);
            if (initial_dist2 >= (wheel_r_inner - 1) * (wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1) * (wheel_r_outer + 1))
            {
                // Interactive with Hue wheel
                H = ImAtan2(current_off.y, current_off.x) / IM_PI * 0.5f;
                if (H < 0.0f)
                    H += 1.0f;
                value_changed = value_changed_h = true;
            }
            float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
            float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
            if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
            {
                // Interacting with SV triangle
                ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
                if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
                    current_off_unrotated = ImTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
                float uu, vv, ww;
                ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
                V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
                S = ImClamp(uu / V, 0.0001f, 1.0f);
                value_changed = value_changed_sv = true;
            }
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
    }
    else if (flags & ImGuiColorEditFlags_PickerHueBar)
    {
        // SV rectangle logic
        InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
        if (IsItemActive() && !is_readonly)
        {
            S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
            V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            ColorEditRestoreH(col, &H); // Greatly reduces hue jitter and reset to 0 when hue == 255 and color is rapidly modified using SV square.
            value_changed = value_changed_sv = true;
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);

        // Hue bar logic
        SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
        InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
        if (IsItemActive() && !is_readonly)
        {
            H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = value_changed_h = true;
        }
    }

    // Alpha bar logic
    if (alpha_bar)
    {
        SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
        InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
        if (IsItemActive())
        {
            col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = true;
        }
    }
    PopItemFlag(); // ImGuiItemFlags_NoNav

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
    {
        SameLine(0, style.ItemInnerSpacing.x);
        BeginGroup();
    }

    if (!(flags & ImGuiColorEditFlags_NoLabel))
    {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
        {
            if ((flags & ImGuiColorEditFlags_NoSidePreview))
                SameLine(0, style.ItemInnerSpacing.x);
            TextEx(label, label_display_end);
        }
    }

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
    {
        PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
        ImVec4 col_v4(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
        if ((flags & ImGuiColorEditFlags_NoLabel))
            Text("Current");

        ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_AlphaMask_ | ImGuiColorEditFlags_NoTooltip;
        ColorButton("##current", col_v4, (flags & sub_flags_to_forward), ImVec2(square_sz * 3, square_sz * 2));
        if (ref_col != NULL)
        {
            Text("Original");
            ImVec4 ref_col_v4(ref_col[0], ref_col[1], ref_col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : ref_col[3]);
            if (ColorButton("##original", ref_col_v4, (flags & sub_flags_to_forward), ImVec2(square_sz * 3, square_sz * 2)))
            {
                memcpy(col, ref_col, components * sizeof(float));
                value_changed = true;
            }
        }
        PopItemFlag();
        EndGroup();
    }

    // Convert back color to RGB
    if (value_changed_h || value_changed_sv)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
            g.ColorEditSavedHue = H;
            g.ColorEditSavedSat = S;
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));
        }
        else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            col[0] = H;
            col[1] = S;
            col[2] = V;
        }
    }

    // R,G,B and H,S,V slider color editor
    bool value_changed_fix_hue_wrap = false;
    SameLine(0, style.ItemInnerSpacing.x);
    BeginGroup();
    if ((flags & ImGuiColorEditFlags_NoInputs) == 0)
    {
        PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
        ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_AlphaMask_ | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSmallPreview;
        ImGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | ImGuiColorEditFlags_NoPicker;
        if (flags & ImGuiColorEditFlags_DisplayRGB || (flags & ImGuiColorEditFlags_DisplayMask_) == 0)
            ImGui::SeparatorText("RGB: ");
            if (ImGuiCustom::ColorEdit4("##rgb", col, sub_flags | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoAlpha))
            {
                // FIXME: Hackily differentiating using the DragInt (ActiveId != 0 && !ActiveIdAllowOverlap) vs. using the InputText or DropTarget.
                // For the later we don't want to run the hue-wrap canceling code. If you are well versed in HSV picker please provide your input! (See #2050)
                value_changed_fix_hue_wrap = (g.ActiveId != 0 && !g.ActiveIdAllowOverlap);
                value_changed = true;
            }
            if (flags & ImGuiColorEditFlags_DisplayHSV || (flags & ImGuiColorEditFlags_DisplayMask_) == 0) {
                ImGui::SeparatorText("HSV: ");
                value_changed |= ImGuiCustom::ColorEdit4("##hsv", col, sub_flags | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_NoAlpha);
            }
            if (flags & ImGuiColorEditFlags_DisplayHex || (flags & ImGuiColorEditFlags_DisplayMask_) == 0) {
                ImGui::SeparatorText("HEX: ");
                value_changed |= ImGuiCustom::ColorEdit4("##hex", col, sub_flags | ImGuiColorEditFlags_DisplayHex);
            }
        PopItemWidth();
    }
    EndGroup();
    // Try to cancel hue wrap (after ColorEdit4 call), if any
    if (value_changed_fix_hue_wrap && (flags & ImGuiColorEditFlags_InputRGB))
    {
        float new_H, new_S, new_V;
        ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
        if (new_H <= 0 && H > 0)
        {
            if (new_V <= 0 && V != new_V)
                ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
            else if (new_S <= 0)
                ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
        }
    }

    if (value_changed)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            R = col[0];
            G = col[1];
            B = col[2];
            ColorConvertRGBtoHSV(R, G, B, H, S, V);
            ColorEditRestoreHS(col, &H, &S, &V);   // Fix local Hue as display below will use it immediately.
        }
        else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            H = col[0];
            S = col[1];
            V = col[2];
            ColorConvertHSVtoRGB(H, S, V, R, G, B);
        }
    }

    const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
    const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
    const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
    const ImU32 col_midgrey = IM_COL32(128, 128, 128, style_alpha8);
    const ImU32 col_hues[6 + 1] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(0,255,255,style_alpha8), IM_COL32(0,0,255,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

    ImVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
    ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
    ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha)); // Important: this is still including the main rendering/style alpha!!

    ImVec2 sv_cursor_pos;

    if (flags & ImGuiColorEditFlags_PickerHueWheel)
    {
        // Render Hue Wheel
        const float aeps = 0.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
        const int segment_per_arc = ImMax(4, (int)wheel_r_outer / 12);
        for (int n = 0; n < 6; n++)
        {
            const float a0 = (n) / 6.0f * 2.0f * IM_PI - aeps;
            const float a1 = (n + 1.0f) / 6.0f * 2.0f * IM_PI + aeps;
            const int vert_start_idx = draw_list->VtxBuffer.Size;
            draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer) * 0.5f, a0, a1, segment_per_arc);
            draw_list->PathStroke(col_white, 0, wheel_thickness);
            const int vert_end_idx = draw_list->VtxBuffer.Size;

            // Paint colors over existing vertices
            ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
            ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
            ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, col_hues[n], col_hues[n + 1]);
        }

        // Render Cursor + preview on Hue Wheel
        float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
        float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
        ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f);
        float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
        int hue_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(hue_cursor_rad); // Lock segment count so the +1 one matches others.
        draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, col_midgrey, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, col_white, hue_cursor_segments);

        // Render SV triangle (rotated according to hue)
        ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
        ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
        ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
        ImVec2 uv_white = GetFontTexUvWhitePixel();
        draw_list->PrimReserve(3, 3);
        draw_list->PrimVtx(tra, uv_white, hue_color32);
        draw_list->PrimVtx(trb, uv_white, col_black);
        draw_list->PrimVtx(trc, uv_white, col_white);
        draw_list->AddTriangle(tra, trb, trc, col_midgrey, 1.5f);
        sv_cursor_pos = ImLerp(ImLerp(trc, tra, ImSaturate(S)), trb, ImSaturate(1 - V));
    }
    else if (flags & ImGuiColorEditFlags_PickerHueBar)
    {
        // Render SV Square
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), col_white, hue_color32, hue_color32, col_white);
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0, 0, col_black, col_black);
        RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
        sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * sv_picker_size), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
        sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * sv_picker_size), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

        // Render Hue Bar
        for (int i = 0; i < 6; ++i)
            draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), col_hues[i], col_hues[i], col_hues[i + 1], col_hues[i + 1]);
        float bar0_line_y = IM_ROUND(picker_pos.y + H * sv_picker_size);
        RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
        RenderArrowsForVerticalBar(draw_list, ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    // Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
    float sv_cursor_rad = value_changed_sv ? wheel_thickness * 0.55f : wheel_thickness * 0.40f;
    int sv_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(sv_cursor_rad); // Lock segment count so the +1 one matches others.
    draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, user_col32_striped_of_alpha, sv_cursor_segments);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad + 1, col_midgrey, sv_cursor_segments);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad, col_white, sv_cursor_segments);

    // Render alpha bar
    if (alpha_bar)
    {
        float alpha = ImSaturate(col[3]);
        ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
        RenderColorRectWithAlphaCheckerboard(draw_list, bar1_bb.Min, bar1_bb.Max, 0, bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
        draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, user_col32_striped_of_alpha, user_col32_striped_of_alpha, user_col32_striped_of_alpha & ~IM_COL32_A_MASK, user_col32_striped_of_alpha & ~IM_COL32_A_MASK);
        float bar1_line_y = IM_ROUND(picker_pos.y + (1.0f - alpha) * sv_picker_size);
        RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
        RenderArrowsForVerticalBar(draw_list, ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    EndGroup();

    if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
        value_changed = false;
    if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();

    return value_changed;
}



