#pragma once
#include "Asset.h"
#include "IArchive.h"

#include <vector>
#include <cstdint>

class VideoAsset : public Asset
{
public:
	friend class ImporterVideo;

	VideoAsset() { m_type = AssetType::VIDEO; }
	VideoAsset(AssetId& id) : Asset(id, AssetType::VIDEO) {}

	const std::vector<uint8_t>& getVideoData() const { return m_data; }
	void setVideoData(const std::vector<uint8_t>& data) { m_data = data; }
	void setVideoData(std::vector<uint8_t>&& data) { m_data = std::move(data); }

	bool isValid() const { return !m_data.empty(); }

	void drawUI() override;

	void serialize(IArchive& archive) override;

private:
	std::vector<uint8_t> m_data;
};
