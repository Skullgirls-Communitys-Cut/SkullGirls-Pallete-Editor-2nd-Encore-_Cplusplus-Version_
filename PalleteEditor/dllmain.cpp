// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "MainThread.h"

#pragma region Global_Vars
HANDLE hMainThread = nullptr;
std::atomic<bool> bMainStopThread = false;

#define SG_NAME L"SkullGirls.exe"



bool IsSGLoadThisDLL();

#pragma endregion

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if (!IsSGLoadThisDLL()) {
            MessageBox(NULL, L"This DLL loaded not by Skullgirls!\nPlease, delete d3d9.dll from this folder!", NULL, MB_ICONERROR);
            //FreeLibraryAndExitThread(hModule, 0);
            return FALSE;
        }
        hMainThread = CreateThread(
            nullptr,
            NULL,
            (LPTHREAD_START_ROUTINE)MainThreadProc,
            hModule,
            CREATE_SUSPENDED,
            NULL
        );
        if (hMainThread) {
            // Продолжаем выполнение после полной загрузки DLL
            ResumeThread(hMainThread);
        }
        return TRUE;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        FreeConsole();

        bMainStopThread = true;

        // Ждем завершения UI потока
        if (bMainStopThread) {
            WaitForSingleObject(hMainThread, 5000);
            CloseHandle(hMainThread);
        }
        return TRUE;
    }
    return TRUE;
}

bool IsSGLoadThisDLL() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Извлекаем только имя файла из полного пути
    std::wstring fullPath(exePath);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    std::wstring exeName = (lastSlash == std::wstring::npos) ?
        fullPath : fullPath.substr(lastSlash + 1);

    std::wstring GameName = SG_NAME;
    // Приводим к нижнему регистру
    for (auto& c : exeName) c = towlower(c);
    for (auto& c : GameName) c = towlower(c);

    return exeName.find(GameName) != std::wstring::npos;
}

