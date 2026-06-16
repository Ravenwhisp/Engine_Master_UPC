#pragma once
#include "Metadata.h"

namespace AssetIO
{

bool saveMetaFile(const Metadata& meta, const std::filesystem::path& metaPath);
bool loadMetaFile(const std::filesystem::path& metaPath, Metadata& outMeta);

} // namespace AssetIO
