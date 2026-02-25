#pragma once
#include "EditorWindow.h"
#include <vector>
#include <string>
#include <imgui.h>

class Logger : public EditorWindow
{
public:
    enum class LogType
    {
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR
    };

    struct LogEntry
    {
        std::string message;
        LogType type;
        float timeStamp = 0.0f;
        int count = 1;
        bool selected = false;

        LogEntry(LogType t, const std::string& msg, float time)
            : message(msg), type(t), timeStamp(time) {
        }
    };

    Logger();
    ~Logger();

    const char* getWindowName() const override { return "Console"; }
    void render() override;

    template<typename... Args>
    static void log(const char* file, int line, const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_INFO, file, line, fmt, args...);
    }

    template<typename... Args>
    static void warning(const char* file, int line, const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_WARNING, file, line, fmt, args...);
    }

    template<typename... Args>
    static void error(const char* file, int line, const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_ERROR, file, line, fmt, args...);
    }

    static Logger* Instance();

private:

    template<typename... Args>
    void addLog(LogType type, const char* file, int line, const char* fmt, Args&&... args)
    {
        char messageBuffer[2048];
        std::snprintf(messageBuffer, sizeof(messageBuffer),
            fmt, std::forward<Args>(args)...);

        char finalBuffer[2300];
        std::snprintf(finalBuffer, sizeof(finalBuffer),
            "[%s:%d] %s",
            file, line, messageBuffer);

        addLogEntry(type, finalBuffer);
    }

    void addLogEntry(LogType type, const std::string& text);
    void drawHeader();
    void drawMessages();
    void drawMessage(LogEntry& entry, size_t index);
    void clear();
    void clearSelection();
    void copyToClipboard();

    const char* getPrefix(LogType type);
    ImVec4 getColor(LogType type);

private:

    std::vector<LogEntry> m_items;
    ImGuiTextFilter m_filter;

    bool m_autoScroll = true;

    bool m_showLogs = true;
    bool m_showWarnings = true;
    bool m_showErrors = true;

    bool m_showTimestamps = true;

    int m_maxEntries = 2048;

    static Logger* s_Instance;
};

#define LOG_INFO(...)     Logger::log(__VA_ARGS__)
#define LOG_WARNING(...)  Logger::warning(__VA_ARGS__)
#define LOG_ERROR(...)    Logger::error(__VA_ARGS__)