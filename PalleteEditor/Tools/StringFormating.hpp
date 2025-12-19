#include "pch.h"

namespace StringFormating {
    std::wstring to_lower(const std::wstring& str) {
        std::wstring result = str;
        CharLowerW(&result[0]);
        return result;
    }
}