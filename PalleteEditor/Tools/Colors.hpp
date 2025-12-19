#include "pch.h"


namespace ColorsTools {

    ImU32 SwapRBChannels(ImU32 RGBA)
    {
        // Swap red and blue channels, keep green and alpha the same
        return ((RGBA & 0xFF00FF00) |           // Keep G and A channels
            ((RGBA & 0x000000FF) << 16) |    // Move B to R position
            ((RGBA & 0x00FF0000) >> 16));    // Move R to B position
    }

}