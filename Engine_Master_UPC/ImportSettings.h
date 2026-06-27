#pragma once
#include <memory>
#include "AssetType.h"
#include "ISerializable.h"
#include "IArchive.h"

class ImportSettings : public ISerializable
{
public:
    virtual ~ImportSettings() = default;

    virtual std::unique_ptr<ImportSettings> clone() const = 0;
    virtual const char* getTypeName() const = 0;
    virtual void drawUI() {}

    void serialize(IArchive& archive) override = 0;

    static std::unique_ptr<ImportSettings> CreateForType(AssetType type);
};
