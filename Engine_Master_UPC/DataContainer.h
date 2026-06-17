#pragma once
#include "Asset.h"
#include "IFieldContainer.h"

#include <typeinfo>
#include <cstring>
#include <string>

class ENGINE_API DataContainer : public Asset, public IFieldContainer
{
public:
	DataContainer() = default;
	explicit DataContainer(AssetReference& id)
		: Asset(id, AssetType::DATA_CONTAINER)
	{
	}

	void serialize(IArchive& archive) override;

	virtual void syncFromData() {}
	void drawUI() override;

	const std::string& getStoredTypeName() const { return m_typeName; }

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
	std::string m_typeName;
};