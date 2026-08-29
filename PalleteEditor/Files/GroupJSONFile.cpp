#include "GroupJSONFiles.h"
#include "Config.h"
#include <fstream>
#include <iostream>

GroupColorManager& GroupColorManager::GetInstance() {
    static GroupColorManager instance;
    return instance;
}

bool GroupColorManager::LoadFromFile(const std::string& filePath) {
    if (filePath == "." || filePath.empty()) {
        std::cout << "No file chosen" << std::endl;
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return false;
    }

    try {
        ordered_json j;
        file >> j;

        // Используем swap для очистки с освобождением памяти
        std::unordered_map<std::string, std::vector<ColorGroup>> newGroups;

        for (auto& [charName, groups] : j.items()) {
            std::vector<ColorGroup> charGroups;
            charGroups.reserve(groups.size()); // Резервируем память заранее

            int currentIndex = 1;
            for (auto& [groupName, count] : groups.items()) {
                charGroups.emplace_back(ColorGroup{
                    groupName,
                    currentIndex,
                    count.get<int>()
                    });
                currentIndex += count.get<int>();
            }

            // Используем emplace для перемещения данных
            newGroups.emplace(charName, std::move(charGroups));
        }

        // Атомарная замена данных
        characterGroups.swap(newGroups);
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }

}

const std::vector<ColorGroup>* GroupColorManager::GetGroupsForCharacter(const std::string& charName) const {
    auto it = characterGroups.find(charName);
    if (it != characterGroups.end()) {
        return &it->second;
    }
    return nullptr;
}

void GroupColorManager::Clear() {
    characterGroups.clear();
}

bool GroupColorManager::HasData() const {
    return !characterGroups.empty();
}