#include "Globals.h"
#include "Logger.h"
#include "Application.h"
#include "TimeModule.h"
#include <cstdarg>

Logger* Logger::s_Instance = nullptr;

Logger::Logger()
{
    if (!s_Instance)
        s_Instance = this;

    addLogEntry(LogType::LOG_INFO, "Logger initialized");
}

Logger::~Logger()
{
    if (s_Instance == this)
        s_Instance = nullptr;
}

Logger* Logger::Instance()
{
    return s_Instance;
}

void Logger::clear()
{
    m_items.clear();
}

void Logger::clearSelection()
{
    for (auto& item : m_items)
        item.selected = false;
}

void Logger::copyToClipboard()
{
    std::string clipboard;

    size_t selectedCount = 0;
    for (auto& item : m_items)
        if (item.selected)
            selectedCount++;

    for (auto& item : m_items)
    {
        if (selectedCount > 0)
        {
            if (!item.selected) continue;
        }
        else
        {
            if (!m_filter.PassFilter(item.message.c_str())) continue;
            if ((item.type == LogType::LOG_INFO && !m_showLogs) ||
                (item.type == LogType::LOG_WARNING && !m_showWarnings) ||
                (item.type == LogType::LOG_ERROR && !m_showErrors))
                continue;
        }

        clipboard += item.message + "\n";
    }

    ImGui::SetClipboardText(clipboard.c_str());
}

void Logger::addLogEntry(LogType type, const std::string& text)
{
    float timestamp = 0.0f;
    if (app && app->getTimeModule())
        timestamp = app->getTimeModule()->time();

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
        m_items.erase(m_items.begin());
}

const char* Logger::getPrefix(LogType type)
{
    switch (type)
    {
    case LogType::LOG_INFO: return "[LOG]";
    case LogType::LOG_WARNING: return "[WARN]";
    case LogType::LOG_ERROR: return "[ERROR]";
    default: return "[???]";
    }
}

ImVec4 Logger::getColor(LogType type)
{
    switch (type)
    {
    case LogType::LOG_INFO: return ImVec4(1, 1, 1, 1);
    case LogType::LOG_WARNING: return ImVec4(1, 0.8f, 0.2f, 1);
    case LogType::LOG_ERROR: return ImVec4(1, 0.3f, 0.3f, 1);
    default: return ImVec4(1, 1, 1, 1);
    }
}

void Logger::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), getWindowFlags()))
    {
        ImGui::End();
        return;
    }

    drawHeader();
    ImGui::Separator();
    drawMessages();

    ImGui::End();
}

void Logger::drawHeader()
{
    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();

    if (ImGui::Button("Copy")) copyToClipboard();

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

void Logger::drawMessages()
{
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t i = 0; i < m_items.size(); ++i)
    {
        auto& item = m_items[i];

        if (!m_filter.PassFilter(item.message.c_str())) continue;

        if ((item.type == LogType::LOG_INFO && !m_showLogs) ||
            (item.type == LogType::LOG_WARNING && !m_showWarnings) ||
            (item.type == LogType::LOG_ERROR && !m_showErrors))
            continue;

        drawMessage(item, i);
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

void Logger::drawMessage(LogEntry& entry, size_t index)
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
        label += " (x" + std::to_string(entry.count) + ")";

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

