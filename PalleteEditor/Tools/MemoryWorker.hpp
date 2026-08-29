#pragma once
#include "pch.h"
#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

namespace MemoryWorker {
    // =============== ПРОСТРАНСТВО ИМЕН ДЛЯ ВНУТРЕННЕЙ РЕАЛИЗАЦИИ ===============
    namespace Detail {
        // Структура для ключа кэша
        struct AddressCacheKey {
            HANDLE hProcess;
            uintptr_t baseAddress;
            std::vector<uintptr_t> offsets;

            bool operator==(const AddressCacheKey& other) const {
                return hProcess == other.hProcess &&
                    baseAddress == other.baseAddress &&
                    offsets == other.offsets;
            }
        };

        // Хэшер для ключа кэша
        struct AddressCacheHasher {
            size_t operator()(const AddressCacheKey& key) const {
                size_t hash = std::hash<HANDLE>{}(key.hProcess);
                hash ^= std::hash<uintptr_t>{}(key.baseAddress) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
                for (auto offset : key.offsets) {
                    hash ^= std::hash<uintptr_t>{}(offset)+0x9e3779b9 + (hash << 6) + (hash >> 2);
                }
                return hash;
            }
        };

        // Статические переменные для кэша (inline для C++17+)
        inline std::unordered_map<AddressCacheKey, uintptr_t, AddressCacheHasher> addressCache;
        inline std::mutex cacheMutex;

        // =============== ОБЪЯВЛЕНИЯ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ ===============
        inline bool CalculateFinalAddress(HANDLE hProcess, uintptr_t baseAddress,
            const std::vector<uintptr_t>& offsets, uintptr_t& finalAddress);

        inline bool CalculateFinalAddressCached(HANDLE hProcess, uintptr_t baseAddress,
            const std::vector<uintptr_t>& offsets, uintptr_t& finalAddress, bool useCache = false);

        inline void ClearAddressCache();
    }

    // =============== ОБЪЯВЛЕНИЯ ОСНОВНЫХ ФУНКЦИЙ ===============

    // Чтение
    template<typename T>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, T* result);

    template<typename T>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, T* result, size_t size);

    template<size_t N>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, char(&result)[N]);

    template<typename T, size_t N>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::array<T, N>& result);

    template<typename T>
    bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::vector<T>& result, size_t count);

    // Запись
    template<typename T>
    bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const T& value);

    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const void* data, size_t size);

    template<size_t N>
    bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const char(&data)[N]);

    template<typename T, size_t N>
    bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::array<T, N>& data);

    template<typename T>
    bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::vector<T>& data);

    // Оптимизации с кэшированием
    template<typename T>
    bool WriteProcessMemoryWithOffsetsCached(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const T& value, bool useCache = true);

    inline bool WriteProcessMemoryWithOffsetsCached(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const void* data, size_t size, bool useCache = true);

    // Вспомогательные функции
    inline bool ReadStringWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::string& result, size_t maxLength = 256);

    inline bool WriteStringWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::string& value);

    // Управление кэшем
    inline void ClearAddressCache();

    // =============== РЕАЛИЗАЦИИ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ ===============

    namespace Detail {
        inline bool CalculateFinalAddress(HANDLE hProcess, uintptr_t baseAddress,
            const std::vector<uintptr_t>& offsets, uintptr_t& finalAddress) {

            uintptr_t currentAddress = baseAddress;

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

            finalAddress = currentAddress;
            return true;
        }

        inline bool CalculateFinalAddressCached(HANDLE hProcess, uintptr_t baseAddress,
            const std::vector<uintptr_t>& offsets, uintptr_t& finalAddress, bool useCache) {

            if (useCache && !offsets.empty()) {
                AddressCacheKey key{ hProcess, baseAddress, offsets };

                {
                    std::lock_guard<std::mutex> lock(cacheMutex);
                    auto it = addressCache.find(key);
                    if (it != addressCache.end()) {
                        finalAddress = it->second;
                        return true;
                    }
                }

                if (CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
                    std::lock_guard<std::mutex> lock(cacheMutex);
                    addressCache[key] = finalAddress;
                    return true;
                }
                return false;
            }

            return CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress);
        }

        inline void ClearAddressCache() {
            std::lock_guard<std::mutex> lock(cacheMutex);
            addressCache.clear();
        }
    }

    // =============== РЕАЛИЗАЦИИ ОСНОВНЫХ ФУНКЦИЙ ===============

    // ---------- РЕАЛИЗАЦИИ ФУНКЦИЙ ЧТЕНИЯ ----------

    template<typename T>
    inline bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, T* result) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            result,
            sizeof(T),
            nullptr);
    }

    template<typename T>
    inline bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, T* result, size_t size) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            result,
            size,
            nullptr);
    }

    template<size_t N>
    inline bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, char(&result)[N]) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            result,
            N,
            nullptr);
    }

    template<typename T, size_t N>
    inline bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::array<T, N>& result) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            result.data(),
            N * sizeof(T),
            nullptr);
    }

    template<typename T>
    inline bool ReadProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::vector<T>& result, size_t count) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        result.resize(count);
        return ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            result.data(),
            count * sizeof(T),
            nullptr);
    }

    // ---------- РЕАЛИЗАЦИИ ФУНКЦИЙ ЗАПИСИ ----------

    template<typename T>
    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const T& value) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            &value,
            sizeof(T),
            nullptr);
    }

    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const void* data, size_t size) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            data,
            size,
            nullptr);
    }

    template<size_t N>
    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const char(&data)[N]) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            data,
            N,
            nullptr);
    }

    template<typename T, size_t N>
    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::array<T, N>& data) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            data.data(),
            N * sizeof(T),
            nullptr);
    }

    template<typename T>
    inline bool WriteProcessMemoryWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::vector<T>& data) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            data.data(),
            data.size() * sizeof(T),
            nullptr);
    }

    // ---------- РЕАЛИЗАЦИИ ФУНКЦИЙ С КЭШИРОВАНИЕМ ----------

    template<typename T>
    inline bool WriteProcessMemoryWithOffsetsCached(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const T& value, bool useCache) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddressCached(hProcess, baseAddress, offsets, finalAddress, useCache)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            &value,
            sizeof(T),
            nullptr);
    }

    inline bool WriteProcessMemoryWithOffsetsCached(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const void* data, size_t size, bool useCache) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddressCached(hProcess, baseAddress, offsets, finalAddress, useCache)) {
            return false;
        }

        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            data,
            size,
            nullptr);
    }

    // ---------- РЕАЛИЗАЦИИ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ ----------

    inline bool ReadStringWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, std::string& result, size_t maxLength) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        std::vector<char> buffer(maxLength);
        if (!ReadProcessMemory(hProcess,
            reinterpret_cast<LPCVOID>(finalAddress),
            buffer.data(),
            maxLength,
            nullptr)) {
            return false;
        }

        // Найти нулевой терминатор
        size_t length = 0;
        while (length < maxLength && buffer[length] != '\0') {
            ++length;
        }

        result.assign(buffer.data(), length);
        return true;
    }

    inline bool WriteStringWithOffsets(HANDLE hProcess, uintptr_t baseAddress,
        const std::vector<uintptr_t>& offsets, const std::string& value) {

        uintptr_t finalAddress;
        if (!Detail::CalculateFinalAddress(hProcess, baseAddress, offsets, finalAddress)) {
            return false;
        }

        // +1 для нулевого терминатора
        return WriteProcessMemory(hProcess,
            reinterpret_cast<LPVOID>(finalAddress),
            value.c_str(),
            value.size() + 1,
            nullptr);
    }

    // ---------- РЕАЛИЗАЦИИ ФУНКЦИЙ УПРАВЛЕНИЯ ----------

    inline void ClearAddressCache() {
        Detail::ClearAddressCache();
    }
}