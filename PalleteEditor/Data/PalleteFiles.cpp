#include "tinyfiledialogs.h"
#include "PalleteFiles.h"
#include "Character.h"
#include "pch.h"

bool PalleteFile::LoadFromFile(Character& s_Char) {
    const char* filterPatterns[1] = { "*.pal" };
    const char* filePath = tinyfd_openFileDialog(
        "Load Table",        // заголовок
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
    if (!file.is_open()) {
        std::cerr << "Can't open file" << filePath << std::endl;
        return false;
    }

    char charNameInGame[16] = { 0 };
    char charNameInFile[16] = { 0 };
    strncpy_s(charNameInGame, s_Char.Char_Name.c_str(), 15);
    file.read(charNameInFile, 16);

    if (strcmp(charNameInGame,charNameInFile)) {
        return false;
    }

    uint32_t numOfColors = 0;
    file.read(reinterpret_cast<char*>(&numOfColors), sizeof(numOfColors));

    uint8_t HueShift_inc = 0;
    uint8_t HueShift_int = 0;
    file.read(reinterpret_cast<char*>(&HueShift_inc), 1);
    file.read(reinterpret_cast<char*>(&HueShift_int), 1);
    std::vector<__int32> temp(s_Char.Num_Of_Color);
    if (s_Char.Num_Of_Color > 0) {
        file.read(reinterpret_cast<char*>(temp.data()),
            s_Char.Num_Of_Color * sizeof(__int32));
    }
    s_Char.Character_Colors = std::move(temp);
    return true;
}

bool PalleteFile::SaveToFile(const Character s_Char) {

    const char* filterPatterns[1] = { "*.pal" };
    std::filesystem::path filePath = tinyfd_saveFileDialog(
        "Save Pallete", // ""
        "*.pal", // ""
        1, // 0
        filterPatterns, // NULL | {"*.txt"}
        NULL);
    if (filePath == "") {
        std::cout << "No file choosen" << std::endl;
        return true;
    }
    if (filePath.extension().string().empty()) {
        filePath = filePath.string() + ".pal";
    }
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Can't open file" << filePath << std::endl;
        return false;
    }

    char charName[16] = { 0 };
    strncpy_s(charName, s_Char.Char_Name.c_str(), 15);
    file.write(charName, 16);

    uint32_t numOfColors = s_Char.Num_Of_Color;
    file.write(reinterpret_cast<const char*>(&numOfColors), sizeof(numOfColors));

    uint8_t HueShift_inc = 0;
    uint8_t HueShift_int = 0;
    file.write(reinterpret_cast<const char*>(&HueShift_inc), 1);
    file.write(reinterpret_cast<const char*>(&HueShift_int), 1);
    for (int i = 1; i < s_Char.Num_Of_Color; i++) {
        __int32 color = s_Char.Character_Colors[i];
        file.write(reinterpret_cast<const char*>(&color), sizeof(color));
    }

    file.close();
}