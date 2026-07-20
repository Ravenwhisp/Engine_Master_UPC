#pragma once
#include "Asset.h"
#include "IFieldContainer.h"

#include <string>

#pragma warning(push)
#pragma warning(disable: 4251)

class ENGINE_API DataContainer : public Asset, public IFieldContainer
{
public:
	DataContainer() = default;
	explicit DataContainer(AssetId& id)
		: Asset(id, AssetType::DATA_CONTAINER)
	{
	}

	virtual void syncFromData() {}
	void drawUI() override;

	void serialize(IArchive& archive) override;

	FieldList getExposedFields() const override
	{
		return {};
	}

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
		return m_typeName.empty() ? getTypeName() : m_typeName.c_str();
	}

	const std::string& getStoredType() const { return m_typeName; }

protected:
	std::string m_typeName;
};

#pragma warning(pop)