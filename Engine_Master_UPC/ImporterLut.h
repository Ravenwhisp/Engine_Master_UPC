#pragma once

#include "Extensions.h"
#include "ImporterSource.h"
#include "LutAsset.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

struct LutSourceData
{
    std::vector<uint8_t> data;
};

class ImporterLut : public ImporterSource<LutSourceData, LutAsset, AssetType::LUT>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == LUT_EXTENSION;
    }

    Asset* createAssetInstance(AssetId& id) const override { return new LutAsset(id); }

protected:
    bool loadExternal(const std::filesystem::path& path, LutSourceData& out) override
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return false;

        const std::streamsize size = file.tellg();
        if (size <= 0) return false;

        file.seekg(0, std::ios::beg);
        out.data.resize(static_cast<size_t>(size));
        return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data.data()), size));
    }

    void importTyped(const LutSourceData& source, LutAsset* destination) override
    {
        destination->setData(std::vector<uint8_t>(source.data));
    }
};
