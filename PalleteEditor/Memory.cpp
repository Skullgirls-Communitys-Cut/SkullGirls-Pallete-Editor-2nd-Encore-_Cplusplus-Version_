#include "pch.h"
#include "Memory.h"
#include "Utills.hpp"

namespace Memory {

    DWORD FindProcessId(const std::wstring& targetProcessName) {
        DWORD pid = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return pid; // Возвращаем пустой вектор в случае ошибки
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe); // Важно: инициализировать размер структуры

        if (!Process32First(hSnapshot, &pe)) {
            CloseHandle(hSnapshot);
            return pid;
        }

        do {
            // Сравниваем имя процесса с искомым
            if (Utills::to_lower(pe.szExeFile) == Utills::to_lower(targetProcessName)) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe));

        CloseHandle(hSnapshot);
        return pid; // Возвращает все найденные PID (может быть несколько)
    }

    DWORD GetModuleBaseAddress(DWORD dwProcessId, std::wstring ModuleName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0; // Не удалось создать снимок процессов
        }

        MODULEENTRY32 ModuleEntry32 = { 0 };
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        DWORD dwModuleBaseAddress = 0;

        if (Module32First(hSnapshot, &ModuleEntry32)) {
            do {
                if (Utills::to_lower(ModuleEntry32.szModule) == Utills::to_lower(ModuleName)) {
                    dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }

        CloseHandle(hSnapshot);
        return dwModuleBaseAddress; // Вернет 0, если модуль не найден
    }

}