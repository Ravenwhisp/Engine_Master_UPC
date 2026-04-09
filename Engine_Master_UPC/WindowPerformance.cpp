#include "Globals.h"
#include "WindowPerformance.h"

#include "Application.h"
#include "ModuleTime.h"
#include "PerformanceProfiler.h"

WindowPerformance::WindowPerformance()
{
}

void WindowPerformance::drawInternal()
{
    float deltaTime = app->getModuleTime()->deltaTime();
    float fps = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;

    m_fps.push_back(fps);
    m_milliseconds.push_back(deltaTime * 1000.0f);

    auto fpsL = m_fps.linearized();
    auto msL = m_milliseconds.linearized();

    char title[64];
    sprintf_s(title, sizeof(title), "Framerate %.1f", fpsL.back());
    ImGui::PlotHistogram("##framerate", &fpsL[0], fpsL.size(), 0, title, 0.0f, 100.0f, ImVec2(310, 100));
    sprintf_s(title, sizeof(title), "Milliseconds %0.1f", msL.back());
    ImGui::PlotHistogram("##milliseconds", &msL[0], msL.size(), 0, title, 0.0f, 50.0f, ImVec2(310, 100));

    ImGui::Separator();
    ImGui::TextDisabled("CPU Profiling");

    const auto& data = getPerfData();

    if (ImGui::BeginTable("PerfTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Scope");
        ImGui::TableSetupColumn("Last (ms)");
        ImGui::TableSetupColumn("Avg (ms)");
        ImGui::TableSetupColumn("Max (ms)");
        ImGui::TableHeadersRow();

        for (const auto& [name, d] : data)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", d.lastMs);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", d.avgMs);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.2f", d.maxMs);
        }

        ImGui::EndTable();
    }
}
