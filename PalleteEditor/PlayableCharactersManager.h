#pragma once
#include "pch.h"

#define MAX_PLAYABLE_CHARACTERS 6

struct PlayableCharacter {
    char Char_Name[16] = "Undefined";
    int Max_Palette_Num = -1;
    int Current_Palette_Num = -1;
    int Num_Of_Color = -1;

    std::vector<ImU32> Character_Colors = {};

    ImU32 LineColor = -1;
    ImU32 SuperShadowColor1 = -1;
    ImU32 SuperShadowColor2 = -1;

    char HueShift_Cos = 0;
    char HueShift_Sin = 0;
    // Конструктор для явной инициализации
    PlayableCharacter() {
        strcpy_s(Char_Name, "Undefined");
        Max_Palette_Num = -1;
        Current_Palette_Num = -1;
        Num_Of_Color = -1;

        Character_Colors = {};

        LineColor = -1;
        SuperShadowColor1 = -1;
        SuperShadowColor2 = -1;

        HueShift_Cos = 0;
        HueShift_Sin = 0;
    }

    // Проверяем, валиден ли персонаж
    bool IsValid() const {
        return strcmp(Char_Name, "Undefined") != 0 &&
            strlen(Char_Name) > 0 &&
            Max_Palette_Num >= 0 &&
            Current_Palette_Num >= 0 &&
            Num_Of_Color >= 0;
    }

    // Сбрасываем значения по умолчанию
    void Reset() {
        strcpy_s(Char_Name, "Undefined");
        Max_Palette_Num = -1;
        Current_Palette_Num = -1;
        Num_Of_Color = -1;

        Character_Colors = {};

        LineColor = -1;
        SuperShadowColor1 = -1;
        SuperShadowColor2 = -1;

        HueShift_Cos = 0;
        HueShift_Sin = 0;
    }
};

enum ColorOptionFlag {
    FLAG_LINE_COLOR = 0,
    FLAG_SUPER_SHADOW_1 = 1,
    FLAG_SUPER_SHADOW_2 = 2,
    // В будущем можно добавить:
    // FLAG_SKIN_COLOR = 3,
    // FLAG_HAIR_COLOR = 4,
    // FLAG_EYE_COLOR = 5
};

class PlayableCharactersManager {
private:
    PlayableCharactersManager();
    ~PlayableCharactersManager() = default;

    static PlayableCharactersManager* s_instance;

    // Храним только имена для UI (чтобы знать какие слоты заняты)
    std::optional<std::string> Character_Names[MAX_PLAYABLE_CHARACTERS];

    // Текущий персонаж - отдельный объект
    PlayableCharacter Current_Character;

    int Current_Character_idx;

public:
    PlayableCharactersManager(const PlayableCharactersManager&) = delete;
    PlayableCharactersManager& operator=(const PlayableCharactersManager&) = delete;
    PlayableCharactersManager(PlayableCharactersManager&&) = delete;
    PlayableCharactersManager& operator=(PlayableCharactersManager&&) = delete;

    // Обновляем только имена в слотах (для UI)
    static bool RefreshCharacterSlots();

    // Очищаем данные
    static void ClearCharacterData();

    // Загружаем конкретного персонажа по индексу
    static bool LoadCharacter(int index = instance().Current_Character_idx);

    static PlayableCharactersManager& instance() {
        static PlayableCharactersManager instance;
        s_instance = &instance;
        LOG_VARIABLE_ADDRESS(instance);
        return instance;
    }

    static PlayableCharactersManager* instancePtr() {
        return s_instance;
    }

    // Получаем имена персонажей (для UI)
    const std::optional<std::string>* GetCharacterNames() const { return Character_Names; }

    // Получаем текущего персонажа
    static PlayableCharacter& GetCurrentCharacter() { return instance().Current_Character; }

    static int& GetCurrentCharacterIndex() { return instance().Current_Character_idx; }

    static void SetCurrentCharacterIndex(int New_Index) { 
        LOG_DEBUG("Set new character index", New_Index);
        instance().Current_Character_idx = New_Index; 
    };

    //Editing characters!
    static bool ChangePaletteNumber(int index_of_new_pallete);
    static bool ChangeOptionPaletteColor(ImU32& BGRAcolorValue, ColorOptionFlag flag);
    static bool ChangePaletteColor(int Color_ID, ImU32 &BGRAcolorValue);
};