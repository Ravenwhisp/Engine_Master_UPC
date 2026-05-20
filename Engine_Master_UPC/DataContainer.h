#pragma once
#include "Asset.h"

#pragma warning(push)
#pragma warning(disable: 4251)

#include <rapidjson/document.h>

class ImporterDataContainer;

class ENGINE_API DataContainer : public Asset
{
public:
	friend class ImporterDataContainer;

	DataContainer() = default;
	explicit DataContainer(AssetReference& id)
		: Asset(id, AssetType::DATA_CONTAINER)
	{
		m_data.SetObject();
	}

	virtual rapidjson::Value getJson(rapidjson::Document::AllocatorType& allocator) const;
	virtual bool deserializeJson(const rapidjson::Value& obj);
	void drawUI() override;

	const rapidjson::Document& getData() const { return m_data; }
	rapidjson::Document& getDataMutable() { return m_data; }

	virtual const char* getTypeName() const { return "DataContainer"; }
	virtual const char* getDisplayTypeName() const { return "Data Container"; }
	virtual const char* getFileExtension() const { return ".datacontainer"; }

protected:
	rapidjson::Document m_data;
};

#pragma warning(pop)
