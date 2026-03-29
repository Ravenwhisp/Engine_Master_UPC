#include "Globals.h"
#include "WindowLogger.h"

#include "Application.h"
#include "ModuleTime.h"

WindowLogger* WindowLogger::s_Instance = nullptr;

WindowLogger::WindowLogger()
{
    if (!s_Instance)
    {
        s_Instance = this;
    }

    addLogEntry(LogType::LOG_INFO, "WindowLogger initialized");
}

WindowLogger::~WindowLogger()
{
    if (s_Instance == this)
    {
        s_Instance = nullptr;
    }
}

WindowLogger* WindowLogger::Instance()
{
    return s_Instance;
}

void WindowLogger::clear()
{
    m_items.clear();
}

void WindowLogger::clearSelection()
{
    for (auto& item : m_items)
    {
        item.selected = false;
    }
}

void WindowLogger::copyToClipboard()
{
    std::string clipboard;
    size_t selectedCount = 0;

    for (auto& item : m_items)
    {
        if (item.selected)
        {
            selectedCount++;
        }
    }

    for (auto& item : m_items)
    {
        if (selectedCount > 0)
        {
            if (!item.selected)
            {
                continue;
            }
        }
        else
        {
            if (!m_filter.PassFilter(item.message.c_str()))
            {
                continue;
            }

            if ((item.type == LogType::LOG_INFO && !m_showLogs) || (item.type == LogType::LOG_WARNING && !m_showWarnings) || (item.type == LogType::LOG_ERROR && !m_showErrors))
            {
                continue;
            }
        }

        clipboard += item.message + "\n";
    }

    ImGui::SetClipboardText(clipboard.c_str());
}

void WindowLogger::addLogEntry(LogType type, const std::string& text)
{
    float timestamp = 0.0f;

    if (app && app->getModuleTime())
    {
        timestamp = app->getModuleTime()->time();
    }

    if (!m_items.empty())
    {
        LogEntry& last = m_items.back();

        if (last.message == text && last.type == type)
        {
            last.count++;
            return;
        }
    }

    m_items.emplace_back(type, text, timestamp);

    if ((int)m_items.size() > m_maxEntries)
    {
        m_items.erase(m_items.begin());
    }
}

const char* WindowLogger::getPrefix(LogType type)
{
    switch (type)
    {
    case LogType::LOG_INFO:
        return "[LOG]";
    case LogType::LOG_WARNING:
        return "[WARN]";
    case LogType::LOG_ERROR:
        return "[ERROR]";
    default:
        return "[???]";
    }
}

ImVec4 WindowLogger::getColor(LogType type)
{
    switch (type)
    {
    case LogType::LOG_INFO:
        return ImVec4(1, 1, 1, 1);
    case LogType::LOG_WARNING:
        return ImVec4(1, 0.8f, 0.2f, 1);
    case LogType::LOG_ERROR:
        return ImVec4(1, 0.3f, 0.3f, 1);
    default:
        return ImVec4(1, 1, 1, 1);
    }
}

void WindowLogger::drawInternal()
{
    drawHeader();
    ImGui::Separator();
    drawMessages();
}

void WindowLogger::drawHeader()
{
    if (ImGui::Button("Clear"))
    {
        clear();
    }

    ImGui::SameLine();

    if (ImGui::Button("Copy"))
    {
        copyToClipboard();
    }

    ImGui::SameLine();
    m_filter.Draw("Filter", 200);
    ImGui::Separator();
    ImGui::Checkbox("Logs", &m_showLogs);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &m_showWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &m_showErrors);
    ImGui::SameLine();
    ImGui::Checkbox("Timestamps", &m_showTimestamps);
}

void WindowLogger::drawMessages()
{
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_items.size(); ++i)
    {
        auto& item = m_items[i];

        if (!m_filter.PassFilter(item.message.c_str()))
        {
            continue;
        }

        if ((item.type == LogType::LOG_INFO && !m_showLogs) || (item.type == LogType::LOG_WARNING && !m_showWarnings) || (item.type == LogType::LOG_ERROR && !m_showErrors))
        {
            continue;
        }

        drawMessage(item, i);
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void WindowLogger::drawMessage(LogEntry& entry, size_t index)
{
    ImGui::PushStyleColor(ImGuiCol_Text, getColor(entry.type));

    std::string label;
    label += getPrefix(entry.type);
    label += " ";

    if (m_showTimestamps)
    {
        label += "[" + std::to_string(entry.timeStamp) + "] ";
    }

    label += entry.message;

    if (entry.count > 1)
    {
        label += " (x" + std::to_string(entry.count) + ")";
    }

    label += "###log_" + std::to_string(index);

    if (ImGui::Selectable(label.c_str(), entry.selected))
    {
        if (!ImGui::GetIO().KeyCtrl)
        {
            clearSelection();
            entry.selected = true;
        }
        else
        {
            entry.selected = !entry.selected;
        }
    }

    ImGui::PopStyleColor();
}
