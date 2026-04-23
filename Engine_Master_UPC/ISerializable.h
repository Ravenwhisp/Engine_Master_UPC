#pragma once
#include "rapidjson/document.h"

#include "AssetType.h"

class ISerializable
{
public:
    virtual ~ISerializable() = default;

    virtual bool toJson(rapidjson::Document& doc) const = 0;
    virtual bool fromJson(const rapidjson::Value& root) = 0;
    virtual AssetType getAssetType() const = 0;
};