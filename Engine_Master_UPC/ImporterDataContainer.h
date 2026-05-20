#pragma once
#include "ImporterNative.h"
#include "DataContainer.h"
#include "Extensions.h"

class ImporterDataContainer
	: public ImporterNative<DataContainer, AssetType::DATA_CONTAINER>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		return path.extension().string() == DATA_CONTAINER_EXTENSION;
	}

	Asset* createAssetInstance(AssetReference& uid) const override
	{
		return new DataContainer(uid);
	}

	bool saveNative(const DataContainer* asset, const std::filesystem::path& path);

protected:
	bool     importNative(const std::filesystem::path& path, DataContainer* dst) override;
	uint64_t saveTyped(const DataContainer* source, uint8_t** outBuffer) override;
	void     loadTyped(const uint8_t* buffer, DataContainer* dst) override;
};
