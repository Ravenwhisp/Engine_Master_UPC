#pragma once
#include <memory>
#include "AssetType.h"
#include <rapidjson/document.h>

class ImportSettings
{
public:
    virtual ~ImportSettings() = default;

    virtual void save(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const = 0;
    virtual void load(const rapidjson::Value& obj) = 0;
    virtual std::unique_ptr<ImportSettings> clone() const = 0;
    virtual const char* getTypeName() const = 0;
    virtual void drawUI() {}

    static std::unique_ptr<ImportSettings> CreateForType(AssetType type);
};
