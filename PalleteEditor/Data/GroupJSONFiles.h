#pragma once
#include <json.hpp> // Используйте библиотеку nlohmann/json
using ordered_json = nlohmann::ordered_json;

// Структура для хранения группировки цветов
struct ColorGroup {
    std::string groupName;
    int startIndex; // Начальный индекс в палитре
    int count;      // Количество цветов в группе
};

class GroupColorGroup {
public:
    inline static std::unordered_map<std::string, std::vector<ColorGroup>> characterGroups;
    static bool LoadFromFile();
};

// Для каждого персонажа

