#include "pch.h"
#include "TableReader.h"
#include "tinyfiledialogs.h"


bool AddressTable::LoadFromFile() {
    const char* filterPatterns[1] = { "*.tbl" };
    const char* filePath = tinyfd_openFileDialog(
        "Load Table",        // заголовок
        "",                     // начальна€ директори€
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
    ResetToDefaults();
    file.read(reinterpret_cast<char*>(&s_Base_Adress_For_Delete), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Base_Adress), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_GameStatus), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_Character), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_Name), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_PaletteData), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_CurrentPalette), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_PaletteTotalOffset), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_NumberOfColor), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_ColorCodeOffset), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&s_Offset_HueShiftOffset), sizeof(int32_t));
    if (Base_Adress_For_Delete == 0) {
        file.read(reinterpret_cast<char*>(&s_NEW_Base_Adress_DonotdisplayCHAR), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&s_NEW_Base_Adress_DonotdisplaySHADOWS), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&s_NEW_Base_Adress_Display_SuperShadowforever), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&s_NEW_Offset_LineColor), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&s_NEW_Offset_SuperShadow), sizeof(int32_t));
    }
    return true;
}

void AddressTable::ResetToDefaults() {
    // ¬осстановление значений по умолчанию
    s_Base_Adress_For_Delete = 0x0;
    s_Base_Adress = 0x852178;
    s_Offset_GameStatus = 0x168;
    s_Offset_Character = 0x5E8;
    s_Offset_Name = 0x310;
    s_Offset_PaletteData = 0x328;
    s_Offset_CurrentPalette = 0x330;
    s_Offset_PaletteTotalOffset = 0x0;
    s_Offset_NumberOfColor = 0x4;
    s_Offset_ColorCodeOffset = 0x8;
    s_Offset_HueShiftOffset = 0x2C;
    s_NEW_Base_Adress_DonotdisplayCHAR = 0x185534;
    s_NEW_Base_Adress_DonotdisplaySHADOWS = 0x186060;
    s_NEW_Base_Adress_Display_SuperShadowforever = 0xA479B;
    s_NEW_Offset_LineColor = 0x38;
    s_NEW_Offset_SuperShadow = 0xC;
}