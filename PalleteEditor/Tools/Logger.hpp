#pragma once

#ifdef _WIN32
#ifndef _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#endif
#endif

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>
#include <windows.h>

struct Colors {
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";

    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* BG_RED = "\033[41m";
};

enum class LogLevel {
    DEBUG_LOG,
    GENERAL_LOG,
    INFO_LOG,
    WARNING_LOG,
    ERROR_LOG,
    CRITICAL_LOG
};

class LOGGER {
private:
    LogLevel m_level;
    bool m_useColors;
    std::mutex m_mutex;
    std::string m_name;

    LOGGER(const LOGGER&) = delete;
    LOGGER& operator=(const LOGGER&) = delete;

    // Приватный конструктор для синглтона
    LOGGER() : m_level(
#ifdef _DEBUG
        LogLevel::DEBUG_LOG
#else
        LogLevel::GENERAL_LOG
#endif
    ), m_useColors(true), m_name("Global") {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    // Публичный конструктор для локальных логгеров
    LOGGER(const std::string& name, LogLevel level, bool useColors)
        : m_level(level), m_useColors(useColors), m_name(name) {
        initConsole();
    }

    void initConsole() {
#ifdef _WIN32
        if (m_useColors) {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                GetConsoleMode(hOut, &dwMode);
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
#endif
    }

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;

        // Используем безопасную версию localtime_s
        std::tm tm_struct;

        errno_t err = localtime_s(&tm_struct, &time);
        if (err != 0) {
            return "[TIME_ERROR]";
        }

        ss << std::put_time(&tm_struct, "%H:%M:%S");
        return ss.str();
    }

    std::string getLevelColor(LogLevel level) {
        if (!m_useColors) return "";

        switch (level) {
        case LogLevel::DEBUG_LOG:    return Colors::CYAN;
        case LogLevel::GENERAL_LOG:  return Colors::WHITE;
        case LogLevel::INFO_LOG:     return Colors::GREEN;
        case LogLevel::WARNING_LOG:  return Colors::YELLOW;
        case LogLevel::ERROR_LOG:    return std::string(Colors::RED) + Colors::BOLD;
        case LogLevel::CRITICAL_LOG: return std::string(Colors::BOLD) + Colors::WHITE + Colors::BG_RED;
        default: return Colors::RESET;
        }
    }

    std::string getLevelPrefix(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG_LOG:    return "DEBUG";
        case LogLevel::GENERAL_LOG:  return "GENERAL";
        case LogLevel::INFO_LOG:     return "INFO";
        case LogLevel::WARNING_LOG:  return "WARN";
        case LogLevel::ERROR_LOG:    return "ERROR";
        case LogLevel::CRITICAL_LOG: return "CRITICAL";
        default: return "???";
        }
    }

    void logImpl(LogLevel level, const std::string& message) {
        std::string color = getLevelColor(level);
        std::string reset = m_useColors ? Colors::RESET : "";

        auto& output = (level >= LogLevel::ERROR_LOG) ? std::cerr : std::cout;

        std::cout << "[" << getCurrentTime() << "] "
            << color
            << "[" << m_name << "] "
            << "[" << getLevelPrefix(level) << "] "
            << message
            << reset
            << std::endl;
    }

public:
    // Статические методы для глобального логгера
    static LOGGER& getGlobal() {
        static LOGGER instance;
        return instance;
    }

    static void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(getGlobal().m_mutex);
        getGlobal().m_level = level;
    }

    static LogLevel getLevel() {
        std::lock_guard<std::mutex> lock(getGlobal().m_mutex);
        return getGlobal().m_level;
    }

    static void enableColors(bool enable) {
        std::lock_guard<std::mutex> lock(getGlobal().m_mutex);
        getGlobal().m_useColors = enable;
        getGlobal().initConsole();
    }

    static bool colorsEnabled() {
        std::lock_guard<std::mutex> lock(getGlobal().m_mutex);
        return getGlobal().m_useColors;
    }

    // Метод для создания локальных логгеров
    static std::unique_ptr<LOGGER> createLocal(const std::string& name,
        LogLevel level = LogLevel::GENERAL_LOG,
        bool useColors = true) {
        return std::unique_ptr<LOGGER>(new LOGGER(name, level, useColors));
    }

    // Метод для глобального логгера
    template<typename... Args>
    static void log(LogLevel level, Args... args) {
        std::lock_guard<std::mutex> lock(getGlobal().m_mutex);

        if (level < getGlobal().m_level) return;

        std::ostringstream message;
        ((message << std::forward<Args>(args)), ...);

        getGlobal().logImpl(level, message.str());
    }

    // Метод для локальных логгеров
    template<typename... Args>
    void logLocal(LogLevel level, Args... args) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (level < m_level) return;

        std::ostringstream message;
        ((message << std::forward<Args>(args)), ...);

        logImpl(level, message.str());
    }

    // Геттеры/сеттеры для локальных логгеров
    void setLocalLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_level = level;
    }

    LogLevel getLocalLevel() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }

    void enableLocalColors(bool enable) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_useColors = enable;
        initConsole();
    }

    bool localColorsEnabled() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_useColors;
    }
};

// Макросы для глобального логгера (остались без изменений)
#define LOG_DEBUG(...) do { \
    LOGGER::log(LogLevel::DEBUG_LOG, __VA_ARGS__); \
} while(0)

#define LOG_GENERAL(...) do { \
    LOGGER::log(LogLevel::GENERAL_LOG, __VA_ARGS__); \
} while(0)

#define LOG_INFO(...) do { \
    LOGGER::log(LogLevel::INFO_LOG, __VA_ARGS__); \
} while(0)

#define LOG_WARN(...) do { \
    LOGGER::log(LogLevel::WARNING_LOG, __VA_ARGS__); \
} while(0)

#define LOG_ERROR(...) do { \
    LOGGER::log(LogLevel::ERROR_LOG, __VA_ARGS__); \
} while(0)

#define LOG_CRITICAL(...) do { \
    LOGGER::log(LogLevel::CRITICAL_LOG, __VA_ARGS__); \
} while(0)

#define LOG_VARIABLE(var) LOG_DEBUG(#var ," = ", (var))
#define LOG_VARIABLE_HEX(var) do { \
    std::ostringstream oss; \
    oss << "0x" << std::hex << std::uppercase << (var); \
    LOG_DEBUG(#var, " = ", oss.str()); \
} while(0)
#define LOG_VARIABLE_ADDRESS(var) LOG_DEBUG(#var ," = ", (&var))
#define LOG_FUNCTION_ENTRY() LOG_DEBUG("Entering ", __func__)
#define LOG_FUNCTION_EXIT() LOG_DEBUG("Exiting ", __func__)
#define LOG_THREAD_INFO() LOG_DEBUG("Thread ID: ", std::this_thread::get_id())

// Новые макросы для локальных логгеров
#define LOG_LOCAL_DEBUG(logger, ...) do { \
    (logger)->logLocal(LogLevel::DEBUG_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_GENERAL(logger, ...) do { \
    (logger)->logLocal(LogLevel::GENERAL_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_INFO(logger, ...) do { \
    (logger)->logLocal(LogLevel::INFO_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_WARN(logger, ...) do { \
    (logger)->logLocal(LogLevel::WARNING_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_ERROR(logger, ...) do { \
    (logger)->logLocal(LogLevel::ERROR_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_CRITICAL(logger, ...) do { \
    (logger)->logLocal(LogLevel::CRITICAL_LOG, __VA_ARGS__); \
} while(0)

#define LOG_LOCAL_VARIABLE(logger, var) LOG_LOCAL_DEBUG(logger, #var ," = ", (var))
#define LOG_LOCAL_VARIABLE_HEX(logger, var) do { \
    std::ostringstream oss; \
    oss << "0x" << std::hex << std::uppercase << (var); \
    LOG_LOCAL_DEBUG(logger, #var, " = ", oss.str()); \
} while(0)
#define LOG_LOCAL_VARIABLE_ADDRESS(logger, var) LOG_LOCAL_DEBUG(logger, #var ," = ", (&var))
#define LOG_LOCAL_FUNCTION_ENTRY(logger) LOG_LOCAL_DEBUG(logger, "Entering ", __func__)
#define LOG_LOCAL_FUNCTION_EXIT(logger) LOG_LOCAL_DEBUG(logger, "Exiting ", __func__)
#define LOG_LOCAL_THREAD_INFO(logger) LOG_LOCAL_DEBUG(logger, "Thread ID: ", std::this_thread::get_id())

static bool CreateConsole() {
    FreeConsole();
    if (!AllocConsole())
        return false;

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(hInput, &mode)) {
        mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE | ENABLE_MOUSE_INPUT);
        mode |= ENABLE_EXTENDED_FLAGS | ENABLE_PROCESSED_INPUT;
        SetConsoleMode(hInput, mode);
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        // Delete mouse interaction in console
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
    }
    return true;
}