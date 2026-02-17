#pragma once
#include "EditorWindow.h"

static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = ImGui::MemAlloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }

class Logger: public EditorWindow
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
        LogType type;
        char* text;
        float timeStamp;
        int count;

        LogEntry(LogType t, const char* msg, float time)
			: type(t), timeStamp(time), count(1)
		{
			text = Strdup(msg);
		}

        ~LogEntry()
		{
            if (text) {
                ImGui::MemFree(text);
            }
		}
    };

    Logger();
    ~Logger();

    const char* getWindowName() const override { return "Console"; }
    void render() override;


    template<typename... Args>
    static void log(const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_INFO, "General", fmt, args...);
    }

    template<typename... Args>
    static void warning(const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_WARNING, "Warning", fmt, args...);
    }

    template<typename... Args>
    static void error(const char* fmt, Args... args)
    {
        Instance()->addLog(LogType::LOG_ERROR, "Error", fmt, args...);
    }

    static Logger* Instance();
        
private:
    template<typename... Args>
    void addLog(LogType type, const char* category, const char* fmt, Args... args)
    {
        char buffer[4096];
        std::snprintf(buffer, sizeof(buffer), fmt, args...);
        buffer[sizeof(buffer) - 1] = 0;

        addLogEntry(type, category, buffer);
    }

    void addLogEntry(LogType type, const char* category, const char* text);
    const char* getTypePrefix(LogType type);
    ImU32 getTypeColor(LogType type);


    // Storage
    ImVector<LogEntry*> m_items;
    char                m_inputBuf[256];
    char                m_filterBuf[256];

    // Configuration
    bool    m_autoScroll = true;
    bool    m_showTimestamps = true;
    bool    m_showCategory = true;
    bool    m_wrapText = false;
    int     m_maxEntries = 1000;

    static Logger* s_Instance;
};

#define LOG_INFO(...)     Logger::log(__VA_ARGS__)
#define LOG_WARNING(...)  Logger::warning(__VA_ARGS__)
#define LOG_ERROR(...)    Logger::error(__VA_ARGS__)
