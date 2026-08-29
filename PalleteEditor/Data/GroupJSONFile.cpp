#include "GroupJSONFiles.h"
#include <json.hpp> 
#include "tinyfiledialogs.h"
#include "pch.h"
#include "config.h"

using ordered_json = nlohmann::ordered_json;

bool GroupColorGroup::LoadFromFile() {
    const char* filterPatterns[1] = { "*.json" };
    const char* filePath = tinyfd_openFileDialog(
        "Load JSON (Character parts)",        // заголовок
        "",                     // начальная директория
        1,                      // количество фильтров
        filterPatterns,         // фильтры
        NULL,                   // описание фильтров
        0                       // множественный выбор (0 - нет, 1 - да)
    );
    if (filePath == NULL) {
        std::cout << "No file choosen" << std::endl;
        return true;
    }
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    config::set_string("CharPart", filePath);

    ordered_json j;
    file >> j;

    characterGroups.clear();

    for (auto& [charName, groups] : j.items()) {
        std::vector<ColorGroup> charGroups;
        int currentIndex = 1;
        for (auto& [groupName, count] : groups.items()) {
            ColorGroup g;
            g.groupName = groupName;
            g.startIndex = currentIndex;
            g.count = count;
            charGroups.push_back(g);

            currentIndex += count;
        }

        characterGroups[charName] = charGroups;
    }
    return true;
}