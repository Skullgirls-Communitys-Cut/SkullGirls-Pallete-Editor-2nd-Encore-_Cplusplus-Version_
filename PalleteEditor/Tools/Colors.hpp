#include "pch.h"

namespace ColorsTools {

    inline ImU32 SwapRBChannels(ImU32 RGBA)
    {
        // Swap red and blue channels, keep green and alpha the same
        return ((RGBA & 0xFF00FF00) |           // Keep G and A channels
            ((RGBA & 0x000000FF) << 16) |    // Move B to R position
            ((RGBA & 0x00FF0000) >> 16));    // Move R to B position
    }
    inline ImVec4 HSVtoRGB(float h, float s, float v)
    {
        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
        rgb.w = 1.0f;
        return rgb;
    }
    inline ImVec4 ImU32BGRAtoImVec4RGBA(ImU32 BGRA)
    {
        ImU32 RGBA = SwapRBChannels(BGRA);
        return ImGui::ColorConvertU32ToFloat4(RGBA);
    }
    inline ImU32 ImVec4RGBAtoImU32BGRA(ImVec4 RGBA)
    {
        ImU32 ImU32RGBA = ImGui::ColorConvertFloat4ToU32(RGBA);
        return SwapRBChannels(ImU32RGBA);
    }
}