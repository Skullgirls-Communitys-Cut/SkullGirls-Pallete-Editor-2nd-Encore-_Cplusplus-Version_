#include "pch.h"

#include "PaletteFiles.h"

#include "PlayableCharactersManager.h"

auto PaletteFilesLogger = LOGGER::createLocal("Palette Files", LogLevel::DEBUG_LOG);

bool PalleteFile::LoadFromFile(std::string filePath) {
    auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();

    LOG_LOCAL_DEBUG(PaletteFilesLogger, "We trying to load Palette!");
    LOG_LOCAL_VARIABLE(PaletteFilesLogger, filePath);

    if (filePath == ".") {
        LOG_LOCAL_WARN(PaletteFilesLogger, "No file choosen - path is NULL");
        return false;
    }
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_LOCAL_ERROR(PaletteFilesLogger, "Can't open file");
        return false;
    }

    PlayableCharacter temp_Character = Curent_Char; //We will make a copy so as not to affect the original. 

    try
    {
        file.read(temp_Character.Char_Name, 16);
        if (strcmp(temp_Character.Char_Name, Curent_Char.Char_Name)) {
            LOG_LOCAL_WARN(PaletteFilesLogger, "This palette of another character!");
            return false;
        }
        file.read(reinterpret_cast<char*>(&temp_Character.Num_Of_Color), sizeof(temp_Character.Num_Of_Color));
        file.read(reinterpret_cast<char*>(&temp_Character.HueShift_Cos), 1);
        file.read(reinterpret_cast<char*>(&temp_Character.HueShift_Sin), 1);
        for (int i = 1; i < temp_Character.Num_Of_Color; i++) { //We Load from second (1) color, becouse first color doesn't affect anything
            file.read(reinterpret_cast<char*>(&temp_Character.Character_Colors[i]), sizeof(temp_Character.Character_Colors[i]));
        }
        if (!file.eof()) { //If it's doesn't end of file -> Read more data.

            file.read(reinterpret_cast<char*>(&temp_Character.LineColor), sizeof(temp_Character.LineColor));
            file.read(reinterpret_cast<char*>(&temp_Character.SuperShadowColor1), sizeof(temp_Character.SuperShadowColor1));
            file.read(reinterpret_cast<char*>(&temp_Character.SuperShadowColor2), sizeof(temp_Character.SuperShadowColor2));



        }
        file.close();
    }
    catch (const std::exception& error)
    {
        LOG_LOCAL_ERROR(PaletteFilesLogger, "Can't write to file! Something bad happend");
        LOG_LOCAL_ERROR(PaletteFilesLogger, error.what());
        return false;
    }
    LOG_LOCAL_DEBUG(PaletteFilesLogger, "The first three colors to define the palette.");
    LOG_LOCAL_VARIABLE_HEX(PaletteFilesLogger, temp_Character.Character_Colors[0]);
    LOG_LOCAL_VARIABLE_HEX(PaletteFilesLogger, temp_Character.Character_Colors[1]);
    LOG_LOCAL_VARIABLE_HEX(PaletteFilesLogger, temp_Character.Character_Colors[2]);

    Curent_Char = temp_Character; //Updata local copy, ONLY if reading of files is correct.

    for (int i = 0; i < Curent_Char.Character_Colors.size(); i++) { //Now, write our colors
        PlayableCharactersManager::ChangePaletteColor(i, Curent_Char.Character_Colors[i]);
    }

    PlayableCharactersManager::ChangeOptionPaletteColor(Curent_Char.LineColor, ColorOptionFlag::FLAG_LINE_COLOR);
    PlayableCharactersManager::ChangeOptionPaletteColor(Curent_Char.SuperShadowColor1, ColorOptionFlag::FLAG_SUPER_SHADOW_1);
    PlayableCharactersManager::ChangeOptionPaletteColor(Curent_Char.SuperShadowColor2, ColorOptionFlag::FLAG_SUPER_SHADOW_2);

    LOG_LOCAL_DEBUG(PaletteFilesLogger, "We successful load!");
    return true;
}

bool PalleteFile::SaveToFile(std::string filePath) {

    auto& Curent_Char = PlayableCharactersManager::GetCurrentCharacter();

    LOG_LOCAL_DEBUG(PaletteFilesLogger, "We trying to save Palette!");
    LOG_LOCAL_VARIABLE(PaletteFilesLogger, filePath);

    if (filePath == ".") {
        LOG_LOCAL_WARN(PaletteFilesLogger, "No file choosen - path is NULL");
        return false;
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_LOCAL_ERROR(PaletteFilesLogger, "Can't open file");
        return false;
    }

    PlayableCharacter temp_Character = Curent_Char; //We will make a copy so as not to affect the original. 

    try
    {
        file.write(temp_Character.Char_Name, 16);
        file.write(reinterpret_cast<const char*>(&temp_Character.Num_Of_Color), sizeof(temp_Character.Num_Of_Color));
        file.write(reinterpret_cast<const char*>(&temp_Character.HueShift_Cos), 1);
        file.write(reinterpret_cast<const char*>(&temp_Character.HueShift_Sin), 1);
        for (int i = 1; i < temp_Character.Num_Of_Color; i++) { //We save from second (1) color, becouse first color doesn't affect anything
            ImU32 color = temp_Character.Character_Colors[i];
            file.write(reinterpret_cast<const char*>(&color), sizeof(color));
        }
        file.write(reinterpret_cast<const char*>(&temp_Character.LineColor), sizeof(temp_Character.LineColor));
        file.write(reinterpret_cast<const char*>(&temp_Character.SuperShadowColor1), sizeof(temp_Character.SuperShadowColor1));
        file.write(reinterpret_cast<const char*>(&temp_Character.SuperShadowColor2), sizeof(temp_Character.SuperShadowColor2));
        file.close();
    }
    catch (const std::exception& error)
    {
        LOG_LOCAL_ERROR(PaletteFilesLogger, "Can't write to file! Something bad happend");
        LOG_LOCAL_ERROR(PaletteFilesLogger, error.what());
        return false;
    }
    LOG_LOCAL_DEBUG(PaletteFilesLogger, "We successful save!");
    return true;
}