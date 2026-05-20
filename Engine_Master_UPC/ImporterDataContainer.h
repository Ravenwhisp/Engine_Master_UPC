#pragma once
#include "ImporterNative.h"
#include "DataContainer.h"
#include "DataContainerFactory.h"
#include "Extensions.h"

class ImporterDataContainer
	: public ImporterNative<DataContainer, AssetType::DATA_CONTAINER>
{
public:
	bool canImport(const std::filesystem::path& path) const override;

	Asset* createAssetInstance(AssetReference& uid) const override;

	bool saveNative(const DataContainer* asset, const std::filesystem::path& path);

protected:
	bool     importNative(const std::filesystem::path& path, DataContainer* dst) override;
	uint64_t saveTyped(const DataContainer* source, uint8_t** outBuffer) override;
	void     loadTyped(const uint8_t* buffer, DataContainer* dst) override;
};
