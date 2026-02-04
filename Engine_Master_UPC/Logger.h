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

    const char* GetWindowName() const override { return "Console"; }
    void Render() override;


    template<typename... Args>
    static void Log(const char* fmt, Args... args)
    {
        Instance()->AddLog(LogType::LOG_INFO, "General", fmt, args...);
    }

    template<typename... Args>
    static void Warning(const char* fmt, Args... args)
    {
        Instance()->AddLog(LogType::LOG_WARNING, "Warning", fmt, args...);
    }

    template<typename... Args>
    static void Error(const char* fmt, Args... args)
    {
        Instance()->AddLog(LogType::LOG_ERROR, "Error", fmt, args...);
    }

    static Logger* Instance();
        
private:
    template<typename... Args>
    void AddLog(LogType type, const char* category, const char* fmt, Args... args)
    {
        char buffer[4096];
        va_list argsList;
        va_start(argsList, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, argsList);
        buffer[sizeof(buffer) - 1] = 0;
        va_end(argsList);

        AddLogEntry(type, category, buffer);
    }

    void AddLogEntry(LogType type, const char* category, const char* text);
    const char* GetTypePrefix(LogType type);
    ImU32 GetTypeColor(LogType type);


    // Storage
    ImVector<LogEntry*> m_Items;
    char m_InputBuf[256];
    char m_FilterBuf[256];

    // Configuration
    bool m_AutoScroll = true;
    bool m_ShowTimestamps = true;
    bool m_ShowCategory = true;
    bool m_WrapText = false;
    int m_MaxEntries = 1000;

    static Logger* s_Instance;
};

#define LOG_INFO(...)     Logger::Log(__VA_ARGS__)
#define LOG_WARNING(...)  Logger::Warning(__VA_ARGS__)
#define LOG_ERROR(...)    Logger::Error(__VA_ARGS__)

#define LOG_CAT(category, ...) Logger::Log(category, __VA_ARGS__)

