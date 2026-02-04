#include "Globals.h"
#include "HardwareWindow.h"
#include "Application.h"
#include "D3D12Module.h"

void HardwareWindow::render()
{
	if (!ImGui::Begin(getWindowName(), getOpenPtr(),
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("CPU", ImGuiTreeNodeFlags_DefaultOpen)){ cpu(); }
	if (ImGui::CollapsingHeader("System Memory", ImGuiTreeNodeFlags_DefaultOpen)){ systemVRAM(); }
	if (ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen)) { gpu(); }
	if (ImGui::CollapsingHeader("Video Memory (VRAM)", ImGuiTreeNodeFlags_DefaultOpen)) { vram(); }
	if (ImGui::CollapsingHeader("GPU Capabilities")) { gpuFeatures(); }

	ImGui::End();
}


void HardwareWindow::cpu()
{
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	ImGui::TextDisabled("Processor");
	ImGui::Separator();

	ImGui::Text("Logical Cores");
	ImGui::SameLine(200);
	ImGui::Text("%u", sysInfo.dwNumberOfProcessors);
}


void HardwareWindow::systemVRAM()
{
	MEMORYSTATUSEX statex = {};
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	const float totalMB = statex.ullTotalPhys / (1024.0f * 1024.0f);
	const float availMB = statex.ullAvailPhys / (1024.0f * 1024.0f);
	const float usedMB = totalMB - availMB;

	ImGui::TextDisabled("System RAM");
	ImGui::Separator();

	ImGui::Text("Total");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", totalMB);

	ImGui::Text("Used");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", usedMB);

	ImGui::Text("Available");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", availMB);

	ImGui::ProgressBar(usedMB / totalMB, ImVec2(-1, 0), "Memory Usage");
}


void HardwareWindow::vram()
{
	auto adapter = app->getD3D12Module()->getAdapter();
	DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
	adapter->QueryVideoMemoryInfo(
		0,
		DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
		&info);

	const float budgetMB = info.Budget / (1024.0f * 1024.0f);
	const float usedMB = info.CurrentUsage / (1024.0f * 1024.0f);
	const float reservedMB = info.CurrentReservation / (1024.0f * 1024.0f);
	const float freeMB = budgetMB - usedMB;

	ImGui::TextDisabled("GPU Video Memory");
	ImGui::Separator();

	ImGui::Text("Budget");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", budgetMB);

	ImGui::Text("Used");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", usedMB);

	ImGui::Text("Reserved");
	ImGui::SameLine(200);
	ImGui::Text("%.0f MB", reservedMB);

	ImGui::ProgressBar(usedMB / budgetMB, ImVec2(-1, 0),
		"VRAM Usage");
}


void HardwareWindow::gpu()
{
	auto adapter = app->getD3D12Module()->getAdapter();
	DXGI_ADAPTER_DESC3 desc;
	adapter->GetDesc3(&desc);

	ImGui::TextDisabled("Graphics Adapter");
	ImGui::Separator();

	ImGui::TextWrapped("%ls", desc.Description);

	ImGui::Text("Dedicated VRAM");
	ImGui::SameLine(200);
	ImGui::Text("%llu MB", desc.DedicatedVideoMemory / (1024 * 1024));
}


void HardwareWindow::gpuFeatures()
{
	auto device = app->getD3D12Module()->getDevice();

	D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
	if (FAILED(device->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS,
		&options,
		sizeof(options))))
		return;

	ImGui::TextDisabled("D3D12 Feature Support");
	ImGui::Separator();

	if (ImGui::BeginTable("GPUCaps", 2,
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupColumn("Feature");
		ImGui::TableSetupColumn("Supported");
		ImGui::TableHeadersRow();

		auto BoolText = [](bool v) { return v ? "Yes" : "No"; };

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Double Precision Shader Ops");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text(BoolText(options.DoublePrecisionFloatShaderOps));

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Output Merger Logic Op");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text(BoolText(options.OutputMergerLogicOp));

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Tiled Resources Tier");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%d", options.TiledResourcesTier);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Resource Binding Tier");
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%d", options.ResourceBindingTier);

		ImGui::EndTable();
	}
}

