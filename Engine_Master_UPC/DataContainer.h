#pragma once
#include "Asset.h"
#include "IFieldContainer.h"

#pragma warning(push)
#pragma warning(disable: 4251)

#include <rapidjson/document.h>
#include <typeinfo>
#include <cstring>

class ENGINE_API DataContainer : public Asset, public IFieldContainer
{
public:
	DataContainer() = default;
	explicit DataContainer(AssetReference& id)
		: Asset(id, AssetType::DATA_CONTAINER)
	{
		m_data.SetObject();
	}

	void serialize(IArchive& archive) override;

	virtual void syncFromData() {}
	void drawUI() override;

	const rapidjson::Document& getData() const { return m_data; }
	rapidjson::Document& getDataMutable() { return m_data; }

	virtual const char* getTypeName() const
	{
		const char* name = typeid(*this).name();
#ifdef _MSC_VER
		if (std::strncmp(name, "class ", 6) == 0) return name + 6;
		if (std::strncmp(name, "struct ", 7) == 0) return name + 7;
#endif
		return name;
	}

	virtual const char* getDisplayTypeName() const
	{
		return getTypeName();
	}

protected:
	rapidjson::Document m_data;
};

#pragma warning(pop)