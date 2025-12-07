#include "Config.h"
#include <json.hpp>
#include <fstream>
#include <filesystem>
#include "FileLoad.h"
#include "Auto-Load-Pallete.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

void config::init() {
    if (loaded) return;

#ifdef _WIN32
    char* USERPROFILE = NULL;  // Just a single pointer, not an array
    size_t len;
    errno_t err = _dupenv_s(&USERPROFILE, &len, "USERPROFILE");
    //C:\Users\ImpDi\Documents\Skullgirls
    config_path = fs::path(USERPROFILE) / "Documents" / "Skullgirls" / "PalEditConfig.json";
#else
    char* home = std::getenv("HOME");
    config_path = home ? fs::path(home) / ".config" / "myapp" / "config.json" : "config.json";
#endif

    if (fs::exists(config_path)) {
        std::ifstream file(config_path);
        file >> data;
        std::string CharPartPath = get_string("CharPart");
        std::string Table = get_string("Table");

        if (CharPartPath != "") {
            std::ifstream charFile(CharPartPath); // Переименовал для ясности
            if (!charFile.is_open()) {
                // Файл не открылся - пропускаем обработку
                // Можно добавить сообщение об ошибке
                std::cerr << "Cannot open CharPart file: " << CharPartPath << std::endl;
            }
            else {
                ordered_json j;
                charFile >> j;

                GroupColorGroup::characterGroups.clear();

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
                    GroupColorGroup::characterGroups[charName] = charGroups;
                }
                charFile.close();
            }
        }
        AutoPallete::load();
    }
    else {
        data = {
            {"CharPart", ""},
            {"Table", ""},
            {"AutoLoadPals", json::array()}
        };
        save();
    }
    loaded = true;
}

// Публичные функции
std::string config::get_string(const std::string& key, const std::string& default_val) {
    try {
        return data.value(key, default_val);
    }
    catch (...) {
        return default_val;
    }
}

void config::set_string(const std::string& key, const std::string& value) {
    data[key] = value;
    save();
}

bool config::get_bool(const std::string& key, bool default_val) {
    return data.value(key, default_val);
}

void config::set_bool(const std::string& key, bool value) {
    data[key] = value;
    save();
}


json config::get_json(const std::string& key, const json& default_val) {
    try {
        if (data.contains(key)) {
            return data[key];
        }
        return default_val;
    }
    catch (...) {
        return default_val;
    }
}

void config::set_json(const std::string& key, const json& value) {
    data[key] = value;
    save();
}

void config::save() {
    fs::create_directories(config_path.parent_path());
    std::ofstream file(config_path);
    file << data.dump(4);
    file.close();
}