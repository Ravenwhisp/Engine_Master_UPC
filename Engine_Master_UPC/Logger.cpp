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

    m_inputBuf[0] = '\0';
    m_filterBuf[0] = '\0';

    addLogEntry(LogType::LOG_INFO, "System", "Logger initialized");
}

Logger::~Logger()
{
    if (s_Instance == this)
    {
        s_Instance = nullptr;
    }

    // Clear all entries
    for (int i = 0; i < m_items.Size; i++)
    {
        delete m_items[i];
    }
    m_items.clear();
}

Logger* Logger::Instance()
{
    return s_Instance;
}

void Logger::addLogEntry(LogType type, const char* category, const char* text)
{
    // Get current timestamp
    float timestamp = 0.0f;
    if (app && app->getTimeModule())
    {
        timestamp = 0.0f;
    }

    LogEntry* entry = new LogEntry(type, text, timestamp);
    m_items.push_back(entry);

    // Limit number of entries
    if (m_items.Size > m_maxEntries)
    {
        delete m_items[0];
        m_items.erase(m_items.begin());
    }
}

const char* Logger::getTypePrefix(LogType type)
{
    switch (type)
    {
        case LogType::LOG_INFO:     return "INFO";
        case LogType::LOG_WARNING: return "WARN";
        case LogType::LOG_ERROR:   return "ERROR";
        default:               return "???";
    }
}

ImU32 Logger::getTypeColor(LogType type)
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

void Logger::render() {
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), getWindowFlags()))
    {
        ImGui::End();
        return;
    }

    float height = ImGui::GetContentRegionAvail().y -
        ImGui::GetFrameHeightWithSpacing() -
        ImGui::GetStyle().ItemSpacing.y;

    if (height < 50.0f) height = 50.0f;

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, height), false);

    if (m_wrapText)
    {
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 10.0f);
    }

    bool scrollToBottom = false;

    // Draw log entries
    for (int i = 0; i < m_items.Size; i++)
    {
        LogEntry* entry = m_items[i];

        // Set text color based on type
        ImGui::PushStyleColor(ImGuiCol_Text, getTypeColor(entry->type));

        // Build display text
        ImGui::TextUnformatted("[");
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(getTypePrefix(entry->type));
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted("] ");

        // Timestamp
        if (m_showTimestamps)
        {
            ImGui::SameLine(0, 5);
            ImGui::TextDisabled("%.2f", entry->timeStamp);
            ImGui::SameLine(0, 5);
            ImGui::TextUnformatted("|");
            ImGui::SameLine(0, 5);
        }

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

        if (m_autoScroll && i == m_items.Size - 1)
        {
            scrollToBottom = true;
        }
    }

    if (m_wrapText)
    {
        ImGui::PopTextWrapPos();
    }

    // Auto-scroll to bottom if enabled
    if (scrollToBottom && m_autoScroll)
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
}
