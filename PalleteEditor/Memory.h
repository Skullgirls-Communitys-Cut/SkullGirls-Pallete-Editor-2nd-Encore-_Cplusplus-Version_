#pragma once
#include "pch.h"

namespace Memory{
	DWORD FindProcessId(const std::wstring& targetProcessName);
	DWORD GetModuleBaseAddress(DWORD dwProcessId, std::wstring ModuleName);

    template<typename T>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, T* result) {
        uintptr_t currentAddress = baseAddress;

        // Проходим по всем оффсетам кроме последнего
        for (size_t i = 0; i < offsets.size(); ++i) {
            currentAddress += offsets[i];

            // Если это не последний оффсет, читаем следующий указатель
            if (i < offsets.size() - 1) {
                uintptr_t nextAddress;
                if (!ReadProcessMemory(hProcess,
                    reinterpret_cast<LPCVOID>(currentAddress),
                    &nextAddress,
                    sizeof(nextAddress),
                    nullptr)) {
                    return false;
                }
                currentAddress = nextAddress;
            }
        }

        // Читаем финальное значение по вычисленному адресу
        std::cout << currentAddress;
        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(currentAddress),
            result,
            sizeof(T),
            nullptr);
    }

    // Специализация для чтения строк (ANSI)
    template<>
    inline  bool ReadProcessMemoryWithOffsets<std::string>(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::string* result) {
        uintptr_t currentAddress = baseAddress;
        const size_t MAX_STRING_LENGTH = 4096; // Максимальная длина строки

        // Проходим по всем оффсетам кроме последнего
        for (size_t i = 0; i < offsets.size(); ++i) {
            currentAddress += offsets[i];

            // Если это не последний оффсет, читаем следующий указатель
            if (i < offsets.size() - 1) {
                uintptr_t nextAddress;
                if (!ReadProcessMemory(hProcess,
                    reinterpret_cast<LPCVOID>(currentAddress),
                    &nextAddress,
                    sizeof(nextAddress),
                    nullptr)) {
                    return false;
                }
                currentAddress = nextAddress;
            }
        }

        // Читаем строку посимвольно пока не встретим нулевой терминатор
        char buffer[MAX_STRING_LENGTH];
        SIZE_T bytesRead;

        for (size_t i = 0; i < MAX_STRING_LENGTH; ++i) {
            char currentChar;
            if (!ReadProcessMemory(hProcess,
                reinterpret_cast<LPCVOID>(currentAddress + i),
                &currentChar,
                sizeof(char),
                &bytesRead) || bytesRead != sizeof(char)) {
                return false;
            }

            if (currentChar == '\0') {
                buffer[i] = '\0';
                *result = std::string(buffer, i);
                return true;
            }

            buffer[i] = currentChar;
        }

        // Если не нашли нулевой терминатор в пределах MAX_STRING_LENGTH
        buffer[MAX_STRING_LENGTH - 1] = '\0';
        *result = std::string(buffer);
        return true;
    }


    template<typename T>
    bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const T& value) {
        uintptr_t currentAddress = baseAddress;

        // Проходим по всем оффсетам кроме последнего
        for (size_t i = 0; i < offsets.size(); ++i) {
            currentAddress += offsets[i];

            // Если это не последний оффсет, читаем следующий указатель
            if (i < offsets.size() - 1) {
                uintptr_t nextAddress;
                if (!ReadProcessMemory(hProcess,
                    reinterpret_cast<LPCVOID>(currentAddress),
                    &nextAddress,
                    sizeof(nextAddress),
                    nullptr)) {
                    return false;
                }
                currentAddress = nextAddress;
            }
        }

        // Записываем значение по вычисленному адресу
        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(currentAddress),
            &value,
            sizeof(T),
            nullptr);
    }

}