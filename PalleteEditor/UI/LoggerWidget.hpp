#pragma once

#include "pch.h"

// Класс подписчика для виджета
class LoggerWidgetSubscriber : public ILogSubscriber {
public:
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string timeStr;
        std::string loggerName;
        LogLevel level;
        std::string message;
        std::thread::id threadId;
        std::string levelStr;

#ifdef IMGUI_VERSION
        ImVec4 color;
#endif
    };

    std::deque<LogEntry> m_localHistory;
    std::mutex m_historyMutex;
    static constexpr size_t MAX_LOCAL_HISTORY = 5000;

    // Получает цвет для уровня лога в ImGui
#ifdef IMGUI_VERSION
    static ImVec4 getLevelColorImGui(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG_LOG:    return ImVec4(0.0f, 1.0f, 1.0f, 1.0f);    // Cyan
        case LogLevel::GENERAL_LOG:  return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);    // Light gray
        case LogLevel::INFO_LOG:     return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);    // Green
        case LogLevel::WARNING_LOG:  return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow
        case LogLevel::ERROR_LOG:    return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);    // Red
        case LogLevel::CRITICAL_LOG: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);    // Red (bold)
        default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
#endif

public:
    void onLogMessage(const ::LogEntry& entry) override {
        LogEntry localEntry{
            entry.timestamp,
            entry.timeStr,
            entry.loggerName,
            entry.level,
            entry.message,
            entry.threadId,
            entry.levelStr
#ifdef IMGUI_VERSION
            , getLevelColorImGui(entry.level)
#endif
        };

        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_localHistory.push_back(localEntry);

        if (m_localHistory.size() > MAX_LOCAL_HISTORY) {
            m_localHistory.pop_front();
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_historyMutex);
        m_localHistory.clear();
    }

    size_t getCount() const {
        //std::lock_guard<std::mutex> lock(m_historyMutex);
        return m_localHistory.size();
    }

    std::vector<LogEntry> getHistory() const {
        //std::lock_guard<std::mutex> lock(m_historyMutex);
        return std::vector<LogEntry>(m_localHistory.begin(), m_localHistory.end());
    }
};

class LOGGERWidget {
private:
    struct LogFilters {
        bool showDebug = true;
        bool showGeneral = true;
        bool showInfo = true;
        bool showWarning = true;
        bool showError = true;
        bool showCritical = true;

        // Фильтр по имени логгера
        std::string nameFilter;

        // Фильтр по сообщению
        std::string messageFilter;

        // Фильтр по потоку
        std::string threadFilter;

        // Сортировка
        bool newestFirst = false;

        // Автопрокрутка
        bool autoScroll = true;

        // Показывать только выбранные логгеры
        std::vector<std::string> selectedLoggers;

        // Время отображения
        bool showTime = true;
        bool showLoggerName = true;
        bool showLevel = true;
        bool showThreadId = false;
    };

    std::shared_ptr<LoggerWidgetSubscriber> m_subscriber;
    LogFilters m_filters;

    // Состояние виджета
    bool m_isOpen = false;
    bool m_shouldScrollToBottom = false;
    float m_windowWidth = 800.0f;
    float m_windowHeight = 600.0f;

    // Кэшированные данные для быстрого поиска
    std::vector<std::string> m_uniqueLoggers;
    std::vector<std::thread::id> m_uniqueThreads;
    bool m_cacheDirty = true;

    // Обновляет кэшированные данные
    void updateCache() {
        auto history = m_subscriber->getHistory();

        m_uniqueLoggers.clear();
        m_uniqueThreads.clear();

        for (const auto& entry : history) {
            // Собираем уникальные имена логгеров
            if (std::find(m_uniqueLoggers.begin(), m_uniqueLoggers.end(),
                entry.loggerName) == m_uniqueLoggers.end()) {
                m_uniqueLoggers.push_back(entry.loggerName);
            }

            // Собираем уникальные ID потоков
            if (std::find(m_uniqueThreads.begin(), m_uniqueThreads.end(),
                entry.threadId) == m_uniqueThreads.end()) {
                m_uniqueThreads.push_back(entry.threadId);
            }
        }

        m_cacheDirty = false;
    }

    // Применяет фильтры к записи
    bool applyFilters(const LoggerWidgetSubscriber::LogEntry& entry) const {
        // Фильтр по уровню
        switch (entry.level) {
        case LogLevel::DEBUG_LOG:    if (!m_filters.showDebug) return false; break;
        case LogLevel::GENERAL_LOG:  if (!m_filters.showGeneral) return false; break;
        case LogLevel::INFO_LOG:     if (!m_filters.showInfo) return false; break;
        case LogLevel::WARNING_LOG:  if (!m_filters.showWarning) return false; break;
        case LogLevel::ERROR_LOG:    if (!m_filters.showError) return false; break;
        case LogLevel::CRITICAL_LOG: if (!m_filters.showCritical) return false; break;
        }

        // Фильтр по имени логгера
        if (!m_filters.nameFilter.empty() &&
            entry.loggerName.find(m_filters.nameFilter) == std::string::npos) {
            return false;
        }

        // Фильтр по сообщению
        if (!m_filters.messageFilter.empty() &&
            entry.message.find(m_filters.messageFilter) == std::string::npos) {
            return false;
        }

        // Фильтр по потоку
        if (!m_filters.threadFilter.empty()) {
            std::stringstream ss;
            ss << entry.threadId;
            if (ss.str().find(m_filters.threadFilter) == std::string::npos) {
                return false;
            }
        }

        // Фильтр по выбранным логгерам
        if (!m_filters.selectedLoggers.empty() &&
            std::find(m_filters.selectedLoggers.begin(),
                m_filters.selectedLoggers.end(),
                entry.loggerName) == m_filters.selectedLoggers.end()) {
            return false;
        }

        return true;
    }

public:
    LOGGERWidget() : m_subscriber(std::make_shared<LoggerWidgetSubscriber>()) {
        // Подписываемся на логи из глобального логгера
        LOGGER::subscribe(m_subscriber);

        // Загружаем существующую историю
        auto history = LOGGER::getLogHistory();
        for (const auto& entry : history) {
            m_subscriber->onLogMessage(entry);
        }

        m_cacheDirty = true;
    }

    ~LOGGERWidget() {
        // Отписываемся от логов
        LOGGER::unsubscribe(m_subscriber);
    }

    // Основной метод отрисовки виджета
    void draw(bool* p_open) {
        if (!*p_open) return;

        ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Log Viewer", p_open, ImGuiWindowFlags_NoCollapse)) {
            drawControlPanel();
            drawLogPanel();
        }
        ImGui::End();
    }

    void setWindowSize(float width, float height) {
        m_windowWidth = width;
        m_windowHeight = height;
    }

    // Методы для управления фильтрами
    void setFilterLevel(LogLevel level, bool show) {
        switch (level) {
        case LogLevel::DEBUG_LOG:    m_filters.showDebug = show; break;
        case LogLevel::GENERAL_LOG:  m_filters.showGeneral = show; break;
        case LogLevel::INFO_LOG:     m_filters.showInfo = show; break;
        case LogLevel::WARNING_LOG:  m_filters.showWarning = show; break;
        case LogLevel::ERROR_LOG:    m_filters.showError = show; break;
        case LogLevel::CRITICAL_LOG: m_filters.showCritical = show; break;
        }
    }

    void setNameFilter(const std::string& filter) {
        m_filters.nameFilter = filter;
    }

    void setMessageFilter(const std::string& filter) {
        m_filters.messageFilter = filter;
    }

    void setAutoScroll(bool autoScroll) {
        m_filters.autoScroll = autoScroll;
    }

    void clearFilters() {
        m_filters = LogFilters();
    }

    void clearLogs() {
        m_subscriber->clear();
        m_cacheDirty = true;
    }

    // Получить текущие фильтры
    const LogFilters& getFilters() const { return m_filters; }

    // Получить количество логов
    size_t getLogCount() const {
        return m_subscriber->getCount();
    }

private:
    // Отрисовка панели управления
    void drawControlPanel() {
        if (ImGui::CollapsingHeader("Filters & Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Фильтры по уровням
            ImGui::Text("Level filters:");
            ImGui::SameLine();
            ImGui::Checkbox("Debug", &m_filters.showDebug);
            ImGui::SameLine();
            ImGui::Checkbox("General", &m_filters.showGeneral);
            ImGui::SameLine();
            ImGui::Checkbox("Info", &m_filters.showInfo);
            ImGui::SameLine();
            ImGui::Checkbox("Warn", &m_filters.showWarning);
            ImGui::SameLine();
            ImGui::Checkbox("Error", &m_filters.showError);
            ImGui::SameLine();
            ImGui::Checkbox("Critical", &m_filters.showCritical);

            ImGui::Spacing();

            // Фильтр по имени логгера
            static char nameFilter[256] = "";
            ImGui::Text("Logger name:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##NameFilter", nameFilter, sizeof(nameFilter))) {
                m_filters.nameFilter = nameFilter;
            }

            // Фильтр по сообщению
            static char msgFilter[256] = "";
            ImGui::SameLine();
            ImGui::Text("Message:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##MsgFilter", msgFilter, sizeof(msgFilter))) {
                m_filters.messageFilter = msgFilter;
            }

            ImGui::Spacing();

            // Кнопки управления
            if (ImGui::Button("Clear Logs")) {
                clearLogs();
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Filters")) {
                clearFilters();
                memset(nameFilter, 0, sizeof(nameFilter));
                memset(msgFilter, 0, sizeof(msgFilter));
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto-scroll", &m_filters.autoScroll);
            ImGui::SameLine();
            ImGui::Checkbox("Newest First", &m_filters.newestFirst);
            ImGui::SameLine();
            ImGui::Text("Logs: %zu", getLogCount());

            // Дополнительные опции
            if (ImGui::TreeNode("Display Options")) {
                ImGui::Checkbox("Show Time", &m_filters.showTime);
                ImGui::SameLine();
                ImGui::Checkbox("Show Logger", &m_filters.showLoggerName);
                ImGui::SameLine();
                ImGui::Checkbox("Show Level", &m_filters.showLevel);
                ImGui::SameLine();
                ImGui::Checkbox("Show Thread", &m_filters.showThreadId);
                ImGui::TreePop();
            }

            // Выбор логгеров (если есть уникальные)
            if (m_cacheDirty) {
                updateCache();
            }

            if (!m_uniqueLoggers.empty() && ImGui::TreeNode("Select Loggers")) {
                for (const auto& logger : m_uniqueLoggers) {
                    bool selected = std::find(m_filters.selectedLoggers.begin(),
                        m_filters.selectedLoggers.end(),
                        logger) != m_filters.selectedLoggers.end();
                    if (ImGui::Checkbox(logger.c_str(), &selected)) {
                        if (selected) {
                            m_filters.selectedLoggers.push_back(logger);
                        }
                        else {
                            m_filters.selectedLoggers.erase(
                                std::remove(m_filters.selectedLoggers.begin(),
                                    m_filters.selectedLoggers.end(),
                                    logger),
                                m_filters.selectedLoggers.end()
                            );
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    // Отрисовка панели с логами
    void drawLogPanel() {
        ImGui::BeginChild("LogPanel", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar |
            ImGuiWindowFlags_AlwaysVerticalScrollbar);

        auto history = m_subscriber->getHistory();
        std::vector<LoggerWidgetSubscriber::LogEntry> filteredLogs;

        // Применяем фильтры
        for (const auto& entry : history) {
            if (applyFilters(entry)) {
                filteredLogs.push_back(entry);
            }
        }

        // Сортируем если нужно
        if (m_filters.newestFirst) {
            std::reverse(filteredLogs.begin(), filteredLogs.end());
        }

        // Отображаем логи
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(filteredLogs.size()));

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                const auto& entry = filteredLogs[i];

                ImGui::PushID(i);

                // Цветной текст в зависимости от уровня
#ifdef IMGUI_VERSION
                ImGui::PushStyleColor(ImGuiCol_Text, entry.color);
#endif

                // Форматируем сообщение
                std::stringstream formattedMsg;
                if (m_filters.showTime) {
                    formattedMsg << "[" << entry.timeStr << "] ";
                }
                if (m_filters.showLoggerName) {
                    formattedMsg << "[" << entry.loggerName << "] ";
                }
                if (m_filters.showLevel) {
                    formattedMsg << "[" << entry.levelStr << "] ";
                }
                if (m_filters.showThreadId) {
                    formattedMsg << "[TID:" << std::hash<std::thread::id>{}(entry.threadId) << "] ";
                }
                formattedMsg << entry.message;

                // Отображаем сообщение
                ImGui::TextUnformatted(formattedMsg.str().c_str());

                // Подсказка при наведении
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Timestamp: %s", entry.timeStr.c_str());
                    ImGui::Text("Logger: %s", entry.loggerName.c_str());
                    ImGui::Text("Level: %s", entry.levelStr.c_str());
                    ImGui::Text("Thread: %zu", std::hash<std::thread::id>{}(entry.threadId));
                    ImGui::Text("Message: %s", entry.message.c_str());
                    ImGui::EndTooltip();
                }

#ifdef IMGUI_VERSION
                ImGui::PopStyleColor();
#endif
                ImGui::PopID();
            }
        }
        clipper.End();

        // Автопрокрутка если нужно
        if (m_shouldScrollToBottom && m_filters.autoScroll) {
            ImGui::SetScrollHereY(1.0f);
            m_shouldScrollToBottom = false;
        }

        // Обновляем флаг прокрутки если мы внизу
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
            m_shouldScrollToBottom = false;
        }

        ImGui::EndChild();
    }
};

inline static std::unique_ptr<LOGGERWidget> g_logWidget = std::make_unique<LOGGERWidget>();