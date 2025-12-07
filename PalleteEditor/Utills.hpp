#include "pch.h"
#include <cmath>

namespace Utills {
    inline std::wstring to_lower(const std::wstring& str) {
        std::wstring result = str;
        CharLowerW(&result[0]);
        return result;
    }

    // Color conversion utilities used by multiple widgets
    inline void RGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
    {
        float mx = std::fmax(r, std::fmax(g, b));
        float mn = std::fmin(r, std::fmin(g, b));
        out_v = mx;
        float d = mx - mn;
        out_s = mx == 0.0f ? 0.0f : d / mx;
        if (d == 0.0f) { out_h = 0.0f; return; }
        if (mx == r) out_h = 60.0f * (fmod(((g - b) / d), 6.0f));
        else if (mx == g) out_h = 60.0f * (((b - r) / d) + 2.0f);
        else out_h = 60.0f * (((r - g) / d) + 4.0f);
        if (out_h < 0.0f) out_h += 360.0f;
    }

    inline void HSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
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

    inline void ARGBToFloat4(__int32 cVal, float out[4])
    {
        out[0] = ((cVal >> 16) & 0xFF) / 255.0f;
        out[1] = ((cVal >> 8) & 0xFF) / 255.0f;
        out[2] = (cVal & 0xFF) / 255.0f;
        out[3] = ((cVal >> 24) & 0xFF) / 255.0f;
    }

    inline __int32 Float4ToARGB(float r, float g, float b, float a)
    {
        return (static_cast<__int32>(a * 255.0f) << 24) |
               (static_cast<__int32>(r * 255.0f) << 16) |
               (static_cast<__int32>(g * 255.0f) << 8) |
               (static_cast<__int32>(b * 255.0f));
    }
}