#pragma once
#include <json.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

using ordered_json = nlohmann::ordered_json;

struct ColorGroup {
    std::string groupName;
    int startIndex;
    int count;
};

class GroupColorManager {
private:
    std::unordered_map<std::string, std::vector<ColorGroup>> characterGroups;

    // Приватные конструкторы для Singleton
    GroupColorManager() = default;
    GroupColorManager(const GroupColorManager&) = delete;
    GroupColorManager& operator=(const GroupColorManager&) = delete;

public:
    // Метод для получения единственного экземпляра
    static GroupColorManager& GetInstance();

    // Загрузка данных из файла
    bool LoadFromFile(const std::string& filePath);

    // Получение групп по имени персонажа
    const std::vector<ColorGroup>* GetGroupsForCharacter(const std::string& charName) const;

    // Очистка данных
    void Clear();

    // Проверка наличия данных
    bool HasData() const;

    // Получение всех данных (для отладки или сериализации)
    const auto& GetAllGroups() const { return characterGroups; }
};