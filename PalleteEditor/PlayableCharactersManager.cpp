#include "pch.h"
#include "PlayableCharactersManager.h"
#include "Tools/MemoryWorker.hpp"
#include "ProcessManager.h"
#include "AddressesTable.h"

PlayableCharactersManager::PlayableCharactersManager() {
    Current_Character_idx = -1;
    Current_Character;
}

PlayableCharactersManager* PlayableCharactersManager::s_instance = nullptr;
auto CharManagerLogger = LOGGER::createLocal("Character Manager", LogLevel::DEBUG_LOG);

ProcessManager& procMgr = ProcessManager::instance();

bool PlayableCharactersManager::RefreshCharacterSlots() {
    LOG_LOCAL_FUNCTION_ENTRY(CharManagerLogger);

    for (int n = 0; n < MAX_PLAYABLE_CHARACTERS; n++) {
        char Char_Name[16];
        bool readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
            ProcessManager::instance().s_SG_Process,
            ProcessManager::instance().s_BaseAddress,
            {
                static_cast<uintptr_t>(AddressTable::Base_Adress()),
                static_cast<uintptr_t>(AddressTable::Offset_Character() + n * 4),
                static_cast<uintptr_t>(AddressTable::Offset_Name())
            },
            &Char_Name,
            sizeof(Char_Name)
        );

        if (!readSuccess) {
            LOG_LOCAL_WARN(CharManagerLogger, "Can't read character name at slot ", n);
            instance().Character_Names[n] = std::nullopt;
            continue;
        }

        if (strlen(Char_Name) == 0) {
            LOG_LOCAL_DEBUG(CharManagerLogger, "Slot ", n, " is empty");
            instance().Character_Names[n] = std::nullopt;
            continue;
        }

        LOG_LOCAL_INFO(CharManagerLogger, "Slot ", n, " has character: ", Char_Name);
        instance().Character_Names[n] = std::string(Char_Name);
    }
    return true;
}

void PlayableCharactersManager::ClearCharacterData() {
    for (int i = 0; i < MAX_PLAYABLE_CHARACTERS; i++) {
        instance().Character_Names[i] = std::nullopt;
    }
    instance().Current_Character.Reset();
    instance().Current_Character_idx = -1;
}

bool PlayableCharactersManager::LoadCharacter(int index) {

    LOG_LOCAL_FUNCTION_ENTRY(CharManagerLogger);

    if (index < 0 || index >= MAX_PLAYABLE_CHARACTERS) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Invalid index in LoadCharacter: ", index);
        return false;
    }

    PlayableCharacter character;
    bool readSuccess = false;

    // Read Name
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),
            static_cast<uintptr_t>(AddressTable::Offset_Name())
        },
        &character.Char_Name,
        sizeof(character.Char_Name)
    );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character name at ", index);
        return false;
    }



    // Read Max Palette Num
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::Offset_PaletteTotalOffset()),
        },
        &character.Max_Palette_Num,
        sizeof(character.Max_Palette_Num)
        );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character max palette num at ", index);
        return false;
    }



    // Read Current Palette
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),
            static_cast<uintptr_t>(AddressTable::Offset_CurrentPalette()),
        },
        &character.Current_Palette_Num,
        sizeof(character.Current_Palette_Num)
        );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character current palette num at ", index);
        return false;
    }



    // Read Count of Colors
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::Offset_NumberOfColor()),
        },
        &character.Num_Of_Color,
        sizeof(character.Num_Of_Color)
        );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character count of colors at ", index);
        return false;
    }



    //Now, we read COLORS

    for (int i{ 0 }; i < character.Num_Of_Color; i++) {

        ImU32 colorValue = 0;

        readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
            procMgr.s_SG_Process,
            procMgr.s_BaseAddress,
            {
                    static_cast<uintptr_t>(AddressTable::Base_Adress()),
                    static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),  // Приведение к нужному типу
                    static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
                    static_cast<uintptr_t>(AddressTable::Offset_ColorCodeOffset()),
                    static_cast<uintptr_t>(4 * character.Current_Palette_Num),
                    static_cast<uintptr_t>(4 * i),
            },
            &colorValue,
            sizeof(colorValue)
            );

        if (!readSuccess) {
            LOG_LOCAL_ERROR(CharManagerLogger, "Can't read color ", i,
                " from palette ", character.Current_Palette_Num);
            colorValue = IM_COL32_WHITE; // Белый цвет как fallback
        }
        character.Character_Colors.push_back(colorValue);
    }

    // Read Line Color
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_LineColor()),
            static_cast<uintptr_t>(4 * character.Current_Palette_Num),
        },
        &character.LineColor,
        sizeof(character.LineColor)
        );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character's color line", index);
        return false;
    }
    // Read SuperShadow1
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_SuperShadow()),
            static_cast<uintptr_t>(4 * character.Current_Palette_Num),
            0
        },
        &character.SuperShadowColor1,
        sizeof(character.SuperShadowColor1)
        );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character's Super Shadow 1", index);
        return false;
    }


    // Read SuperShadow2
    readSuccess = MemoryWorker::ReadProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + index * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_SuperShadow()),
            static_cast<uintptr_t>(4 * character.Current_Palette_Num),
            4
        },
        &character.SuperShadowColor2,
        sizeof(character.SuperShadowColor2)
    );

    if (!readSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't read character's Super Shadow 2", index);
        return false;
    }

    instance().Current_Character = character;
    instance().Current_Character_idx = index;

    LOG_LOCAL_INFO(CharManagerLogger, "Successfully loaded character: ", character.Char_Name,
        " from slot ", index);

    return true;
}

bool PlayableCharactersManager::ChangePaletteNumber(int newPaletteIndex) {
    LOG_LOCAL_FUNCTION_ENTRY(CharManagerLogger);
    auto currentCharIndex = PlayableCharactersManager::instance().Current_Character_idx;
    if (currentCharIndex == -1) {
        LOG_LOCAL_ERROR(CharManagerLogger, "No character selected");
        return false;
    }

    auto& currentChar = instance().Current_Character; // Используем ссылку!

    // Проверяем валидность нового индекса
    if (newPaletteIndex < 0 || newPaletteIndex >= currentChar.Max_Palette_Num) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Invalid palette index: ", newPaletteIndex);
        return false;
    }

    // Проверяем, не пытаемся ли установить то же значение
    if (newPaletteIndex == currentChar.Current_Palette_Num) {
        LOG_LOCAL_DEBUG(CharManagerLogger, "Palette already set to ", newPaletteIndex);
        return true;
    }

    ProcessManager& procMgr = ProcessManager::instance();

    // Записываем в память игры
    bool writeSuccess = MemoryWorker::WriteProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + currentCharIndex * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_CurrentPalette()),
        },
        newPaletteIndex
        );

    if (!writeSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Failed to write palette number to memory");
        return false;
    }

    // Обновляем локальную копию ТОЛЬКО после успешной записи
    LoadCharacter();

    LOG_LOCAL_INFO(CharManagerLogger, "Changed palette to ", newPaletteIndex,
        " for character in slot ", currentCharIndex);
    return true;
}

bool PlayableCharactersManager::ChangeOptionPaletteColor(ImU32& BGRAcolorValue, ColorOptionFlag flag) {
    
    bool writeSuccess;
    
    switch (flag)
    {
    case FLAG_LINE_COLOR:

            writeSuccess = MemoryWorker::WriteProcessMemoryWithOffsets(
            procMgr.s_SG_Process,
            procMgr.s_BaseAddress,
            {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + GetCurrentCharacterIndex() * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_LineColor()),
            static_cast<uintptr_t>(4 * GetCurrentCharacter().Current_Palette_Num),
            },
            BGRAcolorValue
        );

        if (!writeSuccess) {
            LOG_LOCAL_ERROR(CharManagerLogger, "Can't write Line Color: ", BGRAcolorValue);
            return false;
        }

        //Change local copy - we don't need read entire character if we change only one color
        instance().Current_Character.LineColor = BGRAcolorValue;

        break;
    case FLAG_SUPER_SHADOW_1:

        writeSuccess = MemoryWorker::WriteProcessMemoryWithOffsets(
            procMgr.s_SG_Process,
            procMgr.s_BaseAddress,
            {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + GetCurrentCharacterIndex() * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_SuperShadow()),
            static_cast<uintptr_t>(4 * GetCurrentCharacter().Current_Palette_Num),
            0
            },
            BGRAcolorValue
            );

        if (!writeSuccess) {
            LOG_LOCAL_ERROR(CharManagerLogger, "Can't write Shadow Color 1: ", BGRAcolorValue);
            return false;
        }

        //Change local copy - we don't need read entire character if we change only one color
        instance().Current_Character.SuperShadowColor1 = BGRAcolorValue;

        break;
    case FLAG_SUPER_SHADOW_2:

            writeSuccess = MemoryWorker::WriteProcessMemoryWithOffsets(
            procMgr.s_SG_Process,
            procMgr.s_BaseAddress,
            {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + GetCurrentCharacterIndex() * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
            static_cast<uintptr_t>(AddressTable::NEW_Offset_SuperShadow()),
            static_cast<uintptr_t>(4 * GetCurrentCharacter().Current_Palette_Num),
            4
            },
            BGRAcolorValue
        );

        if (!writeSuccess) {
            LOG_LOCAL_ERROR(CharManagerLogger, "Can't write Shadow Color 2: ", BGRAcolorValue);
            return false;
        }

        //Change local copy - we don't need read entire character if we change only one color
        instance().Current_Character.SuperShadowColor2 = BGRAcolorValue;

        break;
    default:
        break;
    }
    return true;

}

bool PlayableCharactersManager::ChangePaletteColor(int Color_ID, ImU32& BGRAcolorValue) {

    bool writeSuccess = MemoryWorker::WriteProcessMemoryWithOffsets(
        procMgr.s_SG_Process,
        procMgr.s_BaseAddress,
        {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + GetCurrentCharacterIndex() * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
        static_cast<uintptr_t>(AddressTable::Offset_ColorCodeOffset()),
        static_cast<uintptr_t>(4 * GetCurrentCharacter().Current_Palette_Num),
        static_cast<uintptr_t>(4 * Color_ID)
        },
        BGRAcolorValue
    );

    if (!writeSuccess) {
        LOG_LOCAL_ERROR(CharManagerLogger, "Can't write color ", Color_ID,
            "with value ", BGRAcolorValue);
        return false;
    }

    //Change local copy - we don't need read entire character to change only one color
    instance().Current_Character.Character_Colors[Color_ID] = BGRAcolorValue;

    return true;
}