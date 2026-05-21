#include "Globals.h"
#include "ImporterDataContainer.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"

#include <cstdio>
#include <fstream>

bool ImporterDataContainer::canImport(const std::filesystem::path& path) const
{
	return path.extension().string() == DATA_CONTAINER_EXTENSION;
}

Asset* ImporterDataContainer::createAssetInstance(AssetReference& uid) const
{
	return new DataContainer(uid);
}

bool ImporterDataContainer::saveNative(const DataContainer* asset, const std::filesystem::path& path)
{
	const_cast<DataContainer*>(asset)->syncFromData();

	rapidjson::Document doc;
	rapidjson::Value json = asset->getJson(doc.GetAllocator());
	doc.Swap(json);

	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::ofstream file(path);
	if (!file.is_open())
	{
		DEBUG_ERROR("[ImporterDataContainer] Could not open '%s' for writing.", path.string().c_str());
		return false;
	}

	file << buffer.GetString();
	return file.good();
}

bool ImporterDataContainer::importNative(const std::filesystem::path& path, DataContainer* dst)
{
	if (!dst)
	{
		return false;
	}

	FILE* fp = std::fopen(path.string().c_str(), "rb");
	if (!fp)
	{
		DEBUG_ERROR("[ImporterDataContainer] Could not open '%s'.", path.string().c_str());
		return false;
	}

	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document doc;
	doc.ParseStream(is);
	std::fclose(fp);

	if (doc.HasParseError() || !doc.IsObject())
	{
		DEBUG_ERROR("[ImporterDataContainer] JSON parse error in '%s'.", path.string().c_str());
		return false;
	}

	return dst->deserializeJson(doc);
}

uint64_t ImporterDataContainer::saveTyped(const DataContainer* source, uint8_t** outBuffer)
{
	const_cast<DataContainer*>(source)->syncFromData();

	rapidjson::Document doc;
	rapidjson::Value json = source->getJson(doc.GetAllocator());
	doc.Swap(json);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	const std::string jsonStr = buffer.GetString();
	const uint64_t size = sizeof(uint32_t) + sizeof(uint32_t) + jsonStr.size();

	uint8_t* data = new uint8_t[size];
	BinaryWriter bw(data);

	bw.u32(1);
	bw.string(jsonStr);

	*outBuffer = data;
	return size;
}

void ImporterDataContainer::loadTyped(const uint8_t* buffer, DataContainer* dst)
{
	BinaryReader reader(buffer);

	const uint32_t version = reader.u32();
	const std::string jsonStr = reader.string();

	rapidjson::Document doc;
	doc.Parse(jsonStr.c_str());

	if (!doc.HasParseError() && doc.IsObject())
	{
		dst->deserializeJson(doc);
	}
}
