#include "Globals.h"
#include "VideoAsset.h"

#include "imgui.h"
#include "IArchive.h"

void VideoAsset::drawUI()
{
	ImGui::Text("Video Asset");
	ImGui::Text("Size: %zu bytes", m_data.size());
}

void VideoAsset::serialize(IArchive& archive)
{
	uint32_t dataSize = static_cast<uint32_t>(m_data.size());
	archive.serialize(dataSize, "dataSize");

	if (archive.mode() == ArchiveMode::Input)
		m_data.resize(dataSize);

	archive.serializeRaw(m_data.data(), dataSize, "data");
}
