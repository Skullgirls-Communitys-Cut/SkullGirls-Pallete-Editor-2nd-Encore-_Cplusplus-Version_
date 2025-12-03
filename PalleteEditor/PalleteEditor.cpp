#include "PalleteEditor.h"
#include "Memory.h"
#include "FileLoad.h"

#define GAME_STATUS_MATCH_STARTED 0x4

namespace PatchStuff {
    std::vector<unsigned char> CodeCave = { //"Skullgirls.exe" + 332EC0
    0x60, 0x8B, 0x81, 0x30, 0x03, 0x00, 0x00, 0x8B, 0x88, 0x28, 0x03, 0x00, 0x00, 0x05, 0x30, 0x03,
    0x00, 0x00, 0x89, 0xC2, 0x83, 0xC4, 0x28, 0x58, 0x89, 0xC6, 0x83, 0xEC, 0x2C, 0x6A, 0x00, 0x6B,
    0xFF, 0x56, 0x01, 0xF8, 0x05, 0xEC, 0x06, 0x00, 0x00, 0x50, 0x89, 0xF7, 0x8B, 0x02, 0x50, 0xE8,
    0xEC, 0x49, 0xD5, 0xFF, 0x61, 0x60, 0x8B, 0x81, 0x30, 0x03, 0x00, 0x00, 0x8B, 0x88, 0x28, 0x03,
    0x00, 0x00, 0x05, 0x30, 0x03, 0x00, 0x00, 0x89, 0xC2, 0x83, 0xC4, 0x28, 0x58, 0x89, 0xC6, 0x83,
    0xEC, 0x2C, 0x6A, 0x01, 0x6B, 0xFF, 0x56, 0x01, 0xF8, 0x05, 0x54, 0x10, 0x00, 0x00, 0x50, 0x89,
    0xF7, 0x8B, 0x02, 0x50, 0xE8, 0xB7, 0x49, 0xD5, 0xFF, 0x61, 0xE9, 0x06, 0x38, 0xE5, 0xFF, 0x00
    };
    std::vector<unsigned char> JmpToCodeCave = { //"Skullgirls.exe" + 18672A
        0xE9, 0x91, 0xC7, 0x1A, 0x00 };
};

int PalEdit::FindVectorIndexByID(int id) {
    for (int i = 0; i < PalEdit::Character_Vector.size(); i++) {
        if (PalEdit::Character_Vector[i].ID == id) {
            return i;
        }
    }
    return -1;
}

void PalEdit::Init() {

    s_ProcessId = Memory::FindProcessId(L"Skullgirls.exe");
    if (s_ProcessId != NULL) {
        s_BaseAddress = Memory::GetModuleBaseAddress(s_ProcessId, L"Skullgirls.exe");
        s_SG_Process = OpenProcess(PROCESS_ALL_ACCESS, TRUE, s_ProcessId);
        //Patch game;
        WriteProcessMemory(s_SG_Process, (LPVOID)(s_BaseAddress + 0x332EC0), PatchStuff::CodeCave.data(), PatchStuff::CodeCave.size(), nullptr);
        WriteProcessMemory(s_SG_Process, (LPVOID)(s_BaseAddress + 0x18672A), PatchStuff::JmpToCodeCave.data(), PatchStuff::JmpToCodeCave.size(), nullptr);
        bGameOpenned = true;
    }
    else {
        bGameOpenned = false;
    }
    Memory::ReadProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_GameStatus())  // Приведение к нужному типу
        },
        &s_GameStatus);

    if (s_GameStatus != GAME_STATUS_MATCH_STARTED) {
        current_character_idx = -1;
        Character_Vector.clear();
        bMatchStarted = false;
        return;
    }
    bMatchStarted = true;
    if (bGameOpenned and bMatchStarted and Character_Vector.size() != 0) {
        return;
    }
    Character_Vector.clear();
    for (int n{ 0 }; n < 6; n++) {
        std::string Name;
        Memory::ReadProcessMemoryWithOffsets(
            s_SG_Process,
            s_BaseAddress, {
            static_cast<uintptr_t>(AddressTable::Base_Adress()),
            static_cast<uintptr_t>(AddressTable::Offset_Character() + n * 4),  // Приведение к нужному типу
            static_cast<uintptr_t>(AddressTable::Offset_Name()) },
            &Name
            );
        if (Name != "") {
            Character Ch;
            Ch.Char_Name = Name;
            Ch.ID = n;
            Character_Vector.push_back(Ch);
            std::cout << Name;
        }
    }
}

void PalEdit::Read_Character() {
    int VectorID = FindVectorIndexByID(current_character_idx);
    Memory::ReadProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + current_character_idx * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
        static_cast<uintptr_t>(AddressTable::Offset_PaletteTotalOffset()),
        },
        &Character_Vector[VectorID].Max_Pallete_Num
        );
    Memory::ReadProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + current_character_idx * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_CurrentPalette()),
        },
        &Character_Vector[VectorID].Current_Pallete_Num
        );
    Memory::ReadProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + current_character_idx * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
        static_cast<uintptr_t>(AddressTable::Offset_NumberOfColor()),
        },
        &Character_Vector[VectorID].Num_Of_Color
        );
        Character_Vector[VectorID].Character_Colors.clear();
        for (int bar{ 0 }; bar < Character_Vector[VectorID].Num_Of_Color; bar++) {
            __int32 color;
            Memory::ReadProcessMemoryWithOffsets(
                s_SG_Process,
                s_BaseAddress, {
                static_cast<uintptr_t>(AddressTable::Base_Adress()),
                static_cast<uintptr_t>(AddressTable::Offset_Character() + Character_Vector[VectorID].ID * 4),  // Приведение к нужному типу
                static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
                static_cast<uintptr_t>(AddressTable::Offset_ColorCodeOffset()),
                static_cast<uintptr_t>(4 * Character_Vector[VectorID].Current_Pallete_Num),
                static_cast<uintptr_t>(4 * bar),
                },
                &color
                );
            Character_Vector[VectorID].Character_Colors.push_back(color);
        }
}

void PalEdit::ChangePallete() {
    int VectorID = FindVectorIndexByID(current_character_idx);
    unsigned __int8 New_Pal = static_cast<unsigned __int8>(Character_Vector[VectorID].Current_Pallete_Num);
    Memory::WriteProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + current_character_idx * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_CurrentPalette()),
        },
        New_Pal
        );
}

void PalEdit::ChangeColor(int Color_ID, __int32 colorValue) {
    int VectorID = FindVectorIndexByID(current_character_idx);
    Memory::WriteProcessMemoryWithOffsets(
        s_SG_Process,
        s_BaseAddress, {
        static_cast<uintptr_t>(AddressTable::Base_Adress()),
        static_cast<uintptr_t>(AddressTable::Offset_Character() + current_character_idx * 4),  // Приведение к нужному типу
        static_cast<uintptr_t>(AddressTable::Offset_PaletteData()),
        static_cast<uintptr_t>(AddressTable::Offset_ColorCodeOffset()),
        static_cast<uintptr_t>(4 * Character_Vector[VectorID].Current_Pallete_Num),
        static_cast<uintptr_t>(4 * Color_ID)
        },
        colorValue
        );
}

void PalEdit::ChangeAllColors() {
    Character& currentChar = PalEdit::Character_Vector[PalEdit::FindVectorIndexByID(PalEdit::current_character_idx)];
    for (int i = 1; i < currentChar.Character_Colors.size(); i++) {
        ChangeColor(i, currentChar.Character_Colors[i-1]);
    }
}






