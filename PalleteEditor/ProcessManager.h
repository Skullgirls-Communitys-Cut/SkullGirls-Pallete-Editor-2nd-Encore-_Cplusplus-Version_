#pragma once
#include "pch.h"

class ProcessManager {
private:
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    ProcessManager();
    ~ProcessManager() = default;

    static ProcessManager* s_instance;

    DWORD GetModuleBaseAddress(DWORD dwProcessId, std::wstring ModuleName);

public:
    DWORD s_ProcessId;
    DWORD s_BaseAddress;
    HANDLE s_SG_Process;

    static ProcessManager& instance() {
        static ProcessManager instance;
        s_instance = &instance;
        return instance;
    }

    static ProcessManager* instancePtr() {
        return s_instance;
    }

    bool ReadProcess();
};