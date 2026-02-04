#include "Globals.h"
#include "Logger.h"
#include "Application.h"
#include "TimeModule.h"
#include <imgui_internal.h>

// Initialize static instance
Logger* Logger::s_Instance = nullptr;

Logger::Logger()
{
    // Set as singleton instance
    if (!s_Instance)
    {
        s_Instance = this;
    }

    m_InputBuf[0] = '\0';
    m_FilterBuf[0] = '\0';

    AddLogEntry(LogType::LOG_INFO, "System", "Logger initialized");
}

Logger::~Logger()
{
    if (s_Instance == this)
    {
        s_Instance = nullptr;
    }

    // Clear all entries
    for (int i = 0; i < m_Items.Size; i++)
    {
        delete m_Items[i];
    }
    m_Items.clear();
}

Logger* Logger::Instance()
{
    return s_Instance;
}

void Logger::AddLogEntry(LogType type, const char* category, const char* text)
{
    // Get current timestamp
    float timestamp = 0.0f;
    if (app && app->GetTimeModule())
    {
        timestamp = 0.0f;
    }

    LogEntry* entry = new LogEntry(type, text, timestamp);
    m_Items.push_back(entry);

    // Limit number of entries
    if (m_Items.Size > m_MaxEntries)
    {
        delete m_Items[0];
        m_Items.erase(m_Items.begin());
    }
}

const char* Logger::GetTypePrefix(LogType type)
{
    switch (type)
    {
    case LogType::LOG_INFO:     return "INFO";
    case LogType::LOG_WARNING: return "WARN";
    case LogType::LOG_ERROR:   return "ERROR";
    default:               return "???";
    }
}

ImU32 Logger::GetTypeColor(LogType type)
{
    ImGuiStyle& style = ImGui::GetStyle();

    switch (type)
    {
    case LogType::LOG_INFO:
        return ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

    case LogType::LOG_WARNING:
        return ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.8f, 0.0f, 1.0f));

    case LogType::LOG_ERROR:
        return ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

    default:
        return ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
    }
}

void Logger::Render() {
    if (!ImGui::Begin(GetWindowName(), GetOpenPtr(), GetWindowFlags()))
    {
        ImGui::End();
        return;
    }

    float height = ImGui::GetContentRegionAvail().y -
        ImGui::GetFrameHeightWithSpacing() -
        ImGui::GetStyle().ItemSpacing.y;

    if (height < 50.0f) height = 50.0f;

    // Begin scrolling region
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, height), false);

    // Set text wrap if enabled
    if (m_WrapText)
    {
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 10.0f);
    }

    // Track if we need to auto-scroll
    bool scrollToBottom = false;

    // Draw log entries
    for (int i = 0; i < m_Items.Size; i++)
    {
        LogEntry* entry = m_Items[i];

        // Set text color based on type
        ImGui::PushStyleColor(ImGuiCol_Text, GetTypeColor(entry->type));

        // Build display text
        ImGui::TextUnformatted("[");
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(GetTypePrefix(entry->type));
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] ");

        // Timestamp
        if (m_ShowTimestamps)
        {
            ImGui::SameLine(0, 5);
            ImGui::TextDisabled("%.2f", entry->timeStamp);
            ImGui::SameLine(0, 5);
            ImGui::TextUnformatted("|");
            ImGui::SameLine(0, 5);
        }

        // Message text
        if (entry->count > 1)
        {
            char countText[64];
            sprintf(countText, "%s (x%d)", entry->text, entry->count);
            ImGui::TextUnformatted(countText);
        }
        else
        {
            ImGui::TextUnformatted(entry->text);
        }

        ImGui::PopStyleColor();

        // Handle auto-scroll
        if (m_AutoScroll && i == m_Items.Size - 1)
        {
            scrollToBottom = true;
        }
    }

    // Restore text wrap
    if (m_WrapText)
    {
        ImGui::PopTextWrapPos();
    }

    // Auto-scroll to bottom if enabled
    if (scrollToBottom && m_AutoScroll)
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
}
