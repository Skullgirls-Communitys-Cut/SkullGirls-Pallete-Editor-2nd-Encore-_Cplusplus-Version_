#include "pch.h"

namespace Utills {
    std::wstring to_lower(const std::wstring& str) {
        std::wstring result = str;
        CharLowerW(&result[0]);
        return result;
    }
}